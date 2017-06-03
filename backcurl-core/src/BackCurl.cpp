//
//  BackCurl.cpp
//  Created by Taymindis Woon on 23/5/17.
//  Copyright © 2017 Taymindis Woon. All rights reserved.
//

#include "BackCurl.h"


namespace bcl {

/** If you activate mainLoopCallBack, please make sure bcl::LoopBackFire on the loop thread, for e.g. UI thread **/
void init() {
    curl_global_init(CURL_GLOBAL_ALL);
}

void LoopBackFire() {
    while (!internal::_mainLoopTasks.empty()) {
        auto task(std::move(internal::_mainLoopTasks.front()));
        internal::_mainLoopTasks.pop_front();

        // lock during the task?
        internal::_tasks_mutex.lock();
        task();
        internal::_tasks_mutex.unlock();
    }
}

bool isReady(FutureResponse const& f) {
    return f.wait_for(std::chrono::seconds(0)) == std::future_status::ready;
}


/*
 You should call curl_global_cleanup once for each call you make to curl_global_init, after you are done using libcurl.

 This function is not thread safe. You must not call it when any other thread in the program (i.e. a thread sharing the same memory) is running.
 This doesn't just mean no other thread that is using libcurl. Because curl_global_cleanup calls functions of other libraries
 that are similarly thread unsafe, it could conflict with any other thread that uses these other libraries.
 */
void cleanUp() {
    curl_global_cleanup();
}

// For image purpose
size_t
writeByteCallback(void *contents, size_t size, size_t nmemb, void *userp) {
    size_t realsize = size * nmemb;
    MemoryByte *memBlock = (MemoryByte *)userp;
    memBlock->append((unsigned char*)contents, realsize);

    return realsize;
}

// For Content
size_t
writeContentCallback(void *contents, size_t size, size_t nmemb, void *contentWrapper) {
    size_t realsize = size * nmemb;
    std::string *memBlock = (std::string *)contentWrapper;
    memBlock->append((char*)contents, realsize);

    return realsize;
}

namespace internal {
std::deque<std::function<void()>> _mainLoopTasks;
std::mutex _tasks_mutex;
bool _hasMainLoopCallBack = false;

void BackCurlLoopCallBackChecker(bcl::Response &s) {
    _hasMainLoopCallBack = true;
}

void wrapResponse(bcl::Request &r, bcl::Response &resp) {
    curl_easy_getinfo(r.curl, CURLINFO_RESPONSE_CODE, &resp.code);

    curl_easy_getinfo(r.curl, CURLINFO_TOTAL_TIME, &resp.totalTime);

    char *url = NULL;
    curl_easy_getinfo(r.curl, CURLINFO_EFFECTIVE_URL, &url);
    if (url)
        resp.effectiveUrl = std::string(url);

    char *ct = NULL;
    curl_easy_getinfo(r.curl, CURLINFO_CONTENT_TYPE, &ct);
    if (ct)
        resp.contentType = std::string(ct);

    resp.__body = r.dataPtr;

    resp.curl = r.curl;
}


/*** comment this pattern for that you can return bahavior right this  **/
//    template <typename DataType>
//    auto __execute__(std::function<void(bcl::Request &req)> optsFilter, std::function<void(bcl::Response)> respFilter, CALL_TYPE,
//                     DataType &chunk) -> Response<decltype(chunk)>
void __execute__(std::function<void(bcl::Request &req)> optsFilter, std::function<void(bcl::Response &resp)> responseCallback,
                 internal::CALL_TYPE callType, Request &request, Response &response) {

    CURLcode curlCode;
    curl_slist *headersList = NULL;
    // curl_global_init(CURL_GLOBAL_ALL);

    /* init the curl session */
    request.curl = curl_easy_init();

    try {
        /***ROOT CAUSE MESSAGE***/
        if ((curlCode = curl_easy_setopt(request.curl, CURLOPT_ERRORBUFFER, request.errorBuffer)) != CURLE_OK) {
            fprintf(stderr, "Failed to set error buffer [%d]\n", curlCode);
            throw request.errorBuffer;
        }

        optsFilter(request);

        bcl::Headers &headers = request.headers;

        if (headers.size() > 0) {
            for (size_t i = 0; i < headers.size(); ++i)
                headersList = curl_slist_append(headersList, (headers.at(i).first + ": " + headers.at(i).second).c_str());

            if (curl_easy_setopt(request.curl, CURLOPT_HTTPHEADER, headersList) != CURLE_OK) {
                fprintf(stderr, "Failed to set headers params [%s]\n", request.errorBuffer);
                throw request.errorBuffer;
            }
        }

        /* get it! and check for errors*/
        if (curl_easy_perform(request.curl) != CURLE_OK) {
            throw request.errorBuffer;
        }
    } catch (char const* err) {
        response.error = std::string(err);

    } catch (std::exception& e) {

    }

    bcl::internal::wrapResponse(request, response);

    if (callType == internal::ASYNC_CALL) {
        (*response.__streamRef) += 2; // Expected will destruct 2 more time
        responseCallback(response);
        //                response.close(); temporary close as Response bundle will close smartly
    } else if (callType == internal::MAIN_LOOP_CALLBACK) {
        (*response.__streamRef) += 2; // Expected will destruct 2 more time
        std::function<void()> task(std::bind(responseCallback, response));
        {
            std::lock_guard<std::mutex> lock(_tasks_mutex);
            _mainLoopTasks.push_back(std::move(task));
        }
    } else {
        responseCallback(response);
        //                response.close(); temporary close
    }

    if (headersList) {
        curl_slist_free_all(headersList);
    }

    /* cleanup curl stuff */
    // curl_easy_cleanup(request.curl); // Resp will clean up

    // Not needed, all will be free when out of scope
    //            SDL_free(chunk.memory);
    //        chunk.erase(chunk.begin());

    /* we're done with libcurl, so clean it up */
    // curl_global_cleanup(); // TODO should we free every time??
}
}

// ** Response Bundle ** /
bcl::Response::Response() {
    curl = NULL;
    __streamRef = new int(1);
    __body = NULL;

}

void bcl::Response::close() {
    // delete static_cast<std::remove_pointer<decltype(body)*>::type>(body); //best solution so far
    if (__body)
        streamClose(__body);

    delete __streamRef;
    //        printf("address of CURL is %p\n", curl);
    if (curl)
        curl_easy_cleanup(curl); // TODO please implement clean up curl later by using valgrind got error or not
    __streamRef = NULL;
    __body = NULL;
}

Response& bcl::Response::operator = (const Response& r)
{
    // Assignment operator
    if (this != &r) // Avoid self assignment
    {
        // Decrement the old reference count
        // if reference become zero delete the old data¢
        if (__streamRef != NULL && --(*__streamRef) == 0)
        {
            close();
        }

        // Copy the data and reference pointer
        // and increment the reference count
        __body = r.__body;
        __streamRef = r.__streamRef;
        curl = r.curl;
        (*__streamRef)++;
    }
    return *this;
}

std::function<void(void*)>& bcl::Response::attachStreamClose()  {
    return streamClose;
}

bcl::Response::~Response () {
    if (__streamRef != NULL && --(*__streamRef) == 0) {
        close();
    }
}

}