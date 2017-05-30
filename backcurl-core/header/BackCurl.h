//
//  BackCurl.hpp
//  cppTraining
//
//  Created by Taymindis Woon on 23/5/17.
//  Copyright © 2017 Taymindis Woon. All rights reserved.
//

#ifndef BackCurl_h
#define BackCurl_h

#include <functional>
#include <queue>
#include <stdio.h>
#include <stdint.h>
#include <atomic>
#include <chrono>
#include <deque>
#include <iostream>
#include <mutex>
#include <future>
#include <thread>
#include <string>
#include <vector>
#include <curl/curl.h>

#ifndef EMPTY_BACK_PARAM
#define EMPTY_BACK_PARAM std::vector<std::pair<std::string, std::string>>()
#endif

#ifndef BACKCURL_FWD
#define BACKCURL_FWD(...) ::std::forward<decltype(__VA_ARGS__)>(__VA_ARGS__)
#endif

namespace bcl {
// Declaration ------------------

using Headers = std::vector<std::pair<std::string, std::string>>;


struct Response {
    CURL *curl;
    long code;    
    std::string contentType;
    std::string effectiveUrl;
    double totalTime;
    std::string error;

    // std::vector<std::string> cookies;  // A map-like collection of cookies returned in the reque    
    std::function<void(void*)>& attachStreamClose();

    Response();
    
    template <typename DataType>
    inline DataType* getBody() {
        return static_cast<DataType*>(__body);
    }

    Response& operator = (const Response& r);

    ~Response ();
    /** This is for internal use only **/
    void* __body;
    int *__streamRef;

private:
    void close();
    std::function<void(void*)> streamClose;    
};


struct Request {
    CURL *curl;
    std::string reqUrl;
    std::string postData;
    char errorBuffer[CURL_ERROR_SIZE];

    //        union {
    //            DataType data;
    //            DataType *dataPtr; // Use this if it is allocated memory, you may need to free while respFilter
    //        };

    void* dataPtr; // For function call back data pointer
    Headers headers;

    Request() {
        dataPtr = NULL;
    }

    ~Request() {
    }
};

template <typename OptType>
void setOpts(bcl::Request &req, CURLoption curlOpt, OptType&& optType) {
    if (curl_easy_setopt(req.curl, curlOpt, BACKCURL_FWD(optType)) != CURLE_OK) {
        throw req.errorBuffer;
    };
}

template <typename OptType, typename... OptTypes>
void setOpts(bcl::Request &req, CURLoption a, OptType&& optType, CURLoption b,  OptTypes&&... optTypes) {
    setOpts(req, a, optType);
    setOpts(req, b, BACKCURL_FWD(optTypes)...);
}

using FutureResponse = std::future<bcl::Response>;
using FuturePromise = std::promise<bcl::Response>;

bool isReady(FutureResponse const& f);
inline bool isProcessing(FutureResponse const &f) {
    return f.valid();
}
inline bool hasRequestedAndReady(FutureResponse const &f) {
    return f.valid() && isReady(f);
}
inline bool hasRequestedButNotReady(FutureResponse const &f) {
    return f.valid() && !isReady(f);
}

void init();
void cleanUp();

void LoopBackFire();
    
namespace internal {
enum CALL_TYPE
    : uint8_t {
    MAIN_LOOP_CALLBACK = 0x1A, ASYNC_CALL = 0x1B, SYNC = 0x1C/*, FUTURE_ASYNC = 0x1D*/
};

extern std::deque<std::function<void()>> _mainLoopTasks;
extern std::mutex _tasks_mutex;
extern bool _hasMainLoopCallBack;

void wrapResponse(bcl::Request &r, bcl::Response &resp);

void __execute__(std::function<void(bcl::Request &req)> optsFilter, std::function<void(bcl::Response &resp)> responseCallback,
                 bcl::internal::CALL_TYPE callType, Request &request, Response &response);

void BackCurlLoopCallBackChecker(bcl::Response &s);
    
template <typename DataType>
void __stream_close__(void *ptr, DataType *x) {
    delete static_cast<decltype(x)>(ptr);
}
}

size_t writeMemoryCallback(void *contents, size_t size, size_t nmemb, void *contentWrapper) {
    size_t realsize = size * nmemb;
    std::string *memBlock = (std::string *)contentWrapper;
    memBlock->append((char*)contents, realsize);

    return realsize;
}

template <typename DataType>
void execute(std::function<void(bcl::Request &req)> optsFilter, std::function<void(bcl::Response &resp)> responseCallback, bcl::internal::CALL_TYPE callType = internal::SYNC) {
    bcl::Request req;
    bcl::Response res;
    DataType *dt = new DataType;
    req.dataPtr = dt;

    res.attachStreamClose() = std::bind(&internal::__stream_close__<DataType>, std::placeholders::_1, dt);
    bcl::internal::__execute__(optsFilter, responseCallback, callType, req, res);


}

template <typename DataType>
bcl::Response execute(std::function<void(bcl::Request &req)> optsFilter, bcl::internal::CALL_TYPE callType = internal::SYNC) {
    bcl::Request req;
    bcl::Response res;
    DataType *dt = new DataType;
    req.dataPtr = dt;
    res.attachStreamClose() = std::bind(&internal::__stream_close__<DataType>, std::placeholders::_1, dt);
//    bcl::Response outerResp;
    bcl::internal::__execute__(optsFilter, [](bcl::Response & resp) {
//        outerResp.code = resp.code;
//        outerResp.body = resp.body;
//        outerResp.__streamRef = resp.__streamRef;
//        outerResp.contentType = resp.contentType;
//        outerResp.effectiveUrl = resp.effectiveUrl;
//        outerResp.error = resp.error;
//        outerResp.totalTime = resp.totalTime;
//        outerResp.curl = resp.curl;
//        outerResp.attachStreamClose() = resp.attachStreamClose();
    } , callType, req, res);
    return res;
}

template <typename DataType>
void executeAsync(std::function<void(bcl::Request &req)> reqFilter,  std::function<void(bcl::Response &resp)> responseCallback) {
    std::async(std::launch::async, [&reqFilter, &responseCallback]() {
        execute<DataType>(reqFilter, responseCallback);
    } );
}

template <typename DataType>
auto execFuture(std::function<void(bcl::Request &req)> reqFilter) -> bcl::FutureResponse {
//        FuturePromise p;
//        FutureResponse frp = p.get_future();
//        std::thread( [&p, &reqFilter]{
//            p.set_value_at_thread_exit(execute<DataType>(reqFilter, ASYNC_CALL));
//        }).detach();
//        return frp;
    return std::async(std::launch::async, [&reqFilter]() {
        return execute<DataType>(reqFilter, internal::ASYNC_CALL);
    } );

}


template <typename DataType>
void executeOnUI(std::function<void(bcl::Request &req)> reqFilter, std::function<void(bcl::Response &resp)> responseCallback) {
//        if(internal::_hasMainLoopCallBack) {
    std::thread([&]() {
        bcl::execute<DataType>(reqFilter, responseCallback, internal::MAIN_LOOP_CALLBACK);
    }).detach();
//            std::thread t(bcl::execute<DataType>, reqFilter, responseCallback, internal::MAIN_LOOP_CALLBACK);
//            t.detach();
//        } else {
//            fprintf(stderr, "You have not specified the loop back fire on ui update scope, please put bcl::initUILoop(); when start using bcl, and specified bcl::LoopBackFire(); in update scope");
//        }
}

}
#endif /* BackCurl_hpp */
