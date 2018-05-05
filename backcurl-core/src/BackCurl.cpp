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
        std::unique_lock<std::mutex> lock(internal::_tasks_mutex);
        while (!internal::_mainLoopTasks.empty()) {
            bcl::UniqueResponse response = std::move(internal::_mainLoopTasks.front());
            internal::_mainLoopTasks.pop_front();
            
            // unlock during the individual task
            lock.unlock();
            response->callBack(&(*response));
            lock.lock();
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
        std::deque<bcl::UniqueResponse> _mainLoopTasks;
        std::mutex _tasks_mutex;
        bool _hasMainLoopCallBack = false;
        
        //void BackCurlLoopCallBackChecker(bcl::Response &s) {
        //    _hasMainLoopCallBack = true;
        //}
        
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
        
        //    template <typename DataType>
        //    auto __execute__(std::function<void(bcl::Request &req)> optsFilter, std::function<void(bcl::Response)> respFilter, CALL_TYPE,
        //                     DataType &chunk) -> Response<decltype(chunk)>
        void __execute__(bcl::Request &request, bcl::UniqueResponse response, bcl::internal::CALL_TYPE callType) {
            
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
                
                request.callBack(&request);
                
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
                response->error = std::string(err);
                
            } catch (std::exception& e) {
                
            }
            
            bcl::internal::wrapResponse(request, *response);
            
            
            if (headersList) {
                curl_slist_free_all(headersList);
            }
            
            // delete []request.cbPtr;
            if(callType == internal::MAIN_LOOP_CALLBACK) {
                std::lock_guard<std::mutex> lock(_tasks_mutex);
                _mainLoopTasks.push_back(std::move(response));
            } else {
                response->callBack(&(*response));
            }
        }
        
        void __execute__(bcl::Request &request, bcl::Response &response) {
            
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
                
                request.callBack(&request);
                
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
            
            
            if (headersList) {
                curl_slist_free_all(headersList);
            }
        }
    }
    
    // ** Request Bundle **/
    lmbdaptr& bcl::Request::attach_lmbdaptr() {
        return callBack;
    }
    
    // ** Response Bundle ** /
    bcl::Response::Response(bcl::Args &_args) {
        args = _args;
        curl = NULL;
        __body = NULL;
        cbPtr = NULL;
        streamClose = NULL;
        
    }
    
    bcl::Response::~Response () {
        //    if (__streamRef != NULL && --(*__streamRef) == 0) {
        close();
        //    }
    }
    
    void bcl::Response::close() {
        // delete static_cast<std::remove_pointer<decltype(body)*>::type>(body); //best solution so far
        if (__body && streamClose)
            streamClose(this);
        
        if(args)
            delete []args;
        // if (cbPtr)
        //     delete []cbPtr;
        
        //        printf("address of CURL is %p\n", curl);
        if (curl)
            curl_easy_cleanup(curl); // TODO please implement clean up curl later by using valgrind got error or not
        //        __body = NULL;
    }
    
    lmbdaptr& bcl::Response::attach_lmbdaptr() {
        return callBack;
    }
    
    void bcl::Response::setStreamClose(lmbdaptr lmbdaClz)  {
        streamClose = lmbdaClz;
    }
    
    bcl::Arg::Arg(): type(0) {
        __strRef = new int(1);
    }
    
    bcl::Arg::Arg(Arg* a) : type(a->type) {
        mapVal(*a);
        __strRef = new int(1);
    }
    
    bcl::Arg::Arg(const Arg& a) : type(a.type), __strRef(a.__strRef)
    {
        mapVal(a);
        (*__strRef) ++;
    }
    
    Arg& bcl::Arg::operator = (const Arg& a)
    {
        // Assignment operator
        if (this != &a) // Avoid self assignment
        {
            if (--(*__strRef) == 0) {
                if (type == 5)
                    delete []getStr;
                delete __strRef;
            }
            mapVal(a);
            __strRef = a.__strRef;
            (*__strRef)++;
        }
        
        return *this;
    }
    
    bcl::Arg::~Arg() {
        if (--(*__strRef) == 0) {
            if (type == 5)
                delete []getStr;
            delete __strRef;
        }
    }
    
    void bcl::Arg::mapVal(const Arg &a) {
        switch (a.type) {
            case 1:
                getInt = a.getInt;
                break;
            case 2:
                getFloat = a.getFloat;
                break;
            case 3:
                getLong = a.getLong;
                break;
            case 4:
                getChar = a.getChar;
                break;
            case 5:
                getStr = a.getStr;
                break;
        }
        type = a.type;
    }
    
    
    
    // Request Args
    void setArgs(bcl::Arg *args, int arg) {
        Arg a;
        a.getInt = arg;
        a.type = 1;
        *args = a;
    }
    
    void setArgs(bcl::Arg *args, float arg) {
        Arg a;
        a.getFloat = arg;
        a.type = 2;
        *args = a;
    }
    
    void setArgs(bcl::Arg *args, long arg) {
        Arg a;
        a.getLong = arg;
        a.type = 3;
        *args = a;
    }
    
    void setArgs(bcl::Arg *args, char arg) {
        Arg a;
        a.getChar = arg;
        a.type = 4;
        *args = a;
    }
    
    void setArgs(bcl::Arg *args, const char* arg) {
        Arg a;
        a.getStr = new char[strlen(arg) + 1];
        strcpy(a.getStr, arg);
        a.type = 5;
        *args = a;
    }
    
}

