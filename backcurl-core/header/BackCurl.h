//
//  BackCurl.h
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

struct Arg {
    int type;
    union {
        int getInt;
        float getFloat;
        long getLong;
        char getChar;
        char* getStr;
    };

    Arg();
    Arg(Arg * a);
    Arg(const Arg & a);
    Arg & operator = (const Arg & a);
    ~Arg();
private:
    void forStr(const char* s, bool isNew);
    void mapVal(const Arg & a, bool isNew);
};

using Args =  std::vector<Arg>;

struct Response {
    CURL * curl;
    long code;
    std::string contentType;
    std::string effectiveUrl;
    double totalTime;
    std::string error;

    // std::vector<std::string> cookies;  // A map-like collection of cookies returned in the reque
    std::function<void(void*)> & attachStreamClose();

    Response();

    template <typename DataType>
    inline DataType * getBody() {
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
    char errorBuffer[CURL_ERROR_SIZE];
    bcl::Args args;

    void* dataPtr; // For function call back data pointer
    Headers headers;

    Request(bcl::Args &_args) {
        dataPtr = NULL;
        args = _args;
    }

    ~Request() {
    }
};

void setArgs(bcl::Args &args, int arg);
void setArgs(bcl::Args &args, float arg);
void setArgs(bcl::Args &args, long arg);
void setArgs(bcl::Args &args, char arg);
void setArgs(bcl::Args &args, const char* arg);

template <typename ArgType, typename... ArgTypes>
void setArgs(bcl::Args &args, ArgType&& argType, ArgTypes&&... argTypes) {
    setArgs(args, BACKCURL_FWD(argType));
    setArgs(args, BACKCURL_FWD(argTypes)...);
}

template <typename ArgType>
bcl::Args args(ArgType&& argType) {
    bcl::Args args;
    setArgs(args, BACKCURL_FWD(argType));
    return args;
}

template <typename ArgType, typename... ArgTypes>
bcl::Args args(ArgType&& argType,  ArgTypes&&... argTypes) {
    bcl::Args args;
    setArgs(args, BACKCURL_FWD(argType));
    setArgs(args, BACKCURL_FWD(argTypes)...);
    return args;
}
// End Request Args


// Curl Options
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
// End Curl Options


using FutureResponse = std::future<bcl::Response>;
using FuturePromise = std::promise<bcl::Response>;
using MemoryByte = std::basic_string<unsigned char>;

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

size_t
writeContentCallback(void *contents, size_t size, size_t nmemb, void *contentWrapper);

size_t
writeByteCallback(void *contents, size_t size, size_t nmemb, void *userp);

template <typename DataType>
void execute(std::function<void(bcl::Request &req)> optsFilter, std::function<void(bcl::Response &resp)> responseCallback,
             bcl::internal::CALL_TYPE callType = internal::SYNC, bcl::Args reqArgs = bcl::Args()) {
    bcl::Request req(reqArgs);
    bcl::Response res;
    DataType *dt = new DataType;
    req.dataPtr = dt;

    res.attachStreamClose() = std::bind(&internal::__stream_close__<DataType>, std::placeholders::_1, dt);
    bcl::internal::__execute__(optsFilter, responseCallback, callType, req, res);


}

template <typename DataType>
bcl::Response execute(std::function<void(bcl::Request &req)> optsFilter,
                      bcl::internal::CALL_TYPE callType = internal::SYNC, bcl::Args reqArgs = bcl::Args()) {
    bcl::Request req(reqArgs);
    bcl::Response res;
    DataType *dt = new DataType;
    req.dataPtr = dt;
    res.attachStreamClose() = std::bind(&internal::__stream_close__<DataType>, std::placeholders::_1, dt);
    bcl::internal::__execute__(optsFilter, [](bcl::Response & resp) {
    } , callType, req, res);
    return res;
}

template <typename DataType>
void executeAsync(std::function<void(bcl::Request &req)> reqFilter,  std::function<void(bcl::Response &resp)> responseCallback) {
    std::async(std::launch::async, [reqFilter, responseCallback]() {
        execute<DataType>(reqFilter, responseCallback, internal::ASYNC_CALL);
    } );
}

template <typename DataType>
auto execFuture(std::function<void(bcl::Request &req)> reqFilter) -> bcl::FutureResponse {
    return std::async(std::launch::async, [reqFilter]() {
        return execute<DataType>(reqFilter, internal::ASYNC_CALL);
    } );

}


template <typename DataType>
void executeOnUI(std::function<void(bcl::Request &req)> reqFilter, std::function<void(bcl::Response &resp)> responseCallback) {
    std::thread([reqFilter, responseCallback]() {
        bcl::execute<DataType>(reqFilter, responseCallback, internal::MAIN_LOOP_CALLBACK);
    }).detach();
}


/** with args **/
template <typename DataType>
void executeAsync(std::function<void(bcl::Request &req)> reqFilter, bcl::Args args,  std::function<void(bcl::Response &resp)> responseCallback) {
    std::async(std::launch::async, [reqFilter, responseCallback, args]() {
        execute<DataType>(reqFilter, responseCallback, internal::ASYNC_CALL, args);
    } );
}



template <typename DataType>
auto execFuture(std::function<void(bcl::Request &req)> reqFilter, bcl::Args args) -> bcl::FutureResponse {
    return std::async(std::launch::async, [reqFilter, args]() {
        return execute<DataType>(reqFilter, internal::ASYNC_CALL, args);
    } );

}

template <typename DataType>
void executeOnUI(std::function<void(bcl::Request &req)> reqFilter, bcl::Args args, std::function<void(bcl::Response &resp)> responseCallback) {
    std::thread([reqFilter, responseCallback, args]() {
        bcl::execute<DataType>(reqFilter, responseCallback, internal::MAIN_LOOP_CALLBACK, args);
    }).detach();
}

}
#endif /* BackCurl_h */
