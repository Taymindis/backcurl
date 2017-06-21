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
#include <cstring>
#include <vector>
#include "curl/curl.h"

#ifndef EMPTY_BACK_PARAM
#define EMPTY_BACK_PARAM std::vector<std::pair<std::string, std::string>>()
#endif

#ifndef BACKCURL_FWD
#define BACKCURL_FWD(...) ::std::forward<decltype(__VA_ARGS__)>(__VA_ARGS__)
#endif

namespace bcl {
    // Declaration ------------------
    struct Arg {
        int type;
        union {
            int getInt;
            float getFloat;
            long getLong;
            char getChar;
            char* getStr;
        };
        int *__strRef;
        
        Arg();
        Arg(Arg * a);
        Arg(const Arg & a);
        Arg & operator = (const Arg & a);
        ~Arg();
    private:
        void mapVal(const Arg & a);
    };
    
    using Args = Arg*;
    typedef void (*lmbdaptr)(void *x);
    struct Response {
        CURL * curl;
        long code;
        std::string contentType;
        std::string effectiveUrl;
        double totalTime;
        std::string error;
        bcl::Args args;
        
        // std::vector<std::string> cookies;  // A map-like collection of cookies returned in the reque
        lmbdaptr& attach_lmbdaptr();
        
        void setStreamClose(lmbdaptr lmbdaClz);
        
        Response(bcl::Args &_args);
        
        template <typename DataType = std::string>
        inline DataType * getBody() {
            return static_cast<DataType*>((void*)__body);
        }
        
        ~Response ();
        /** This is for internal use only **/
        void* __body;
        void* cbPtr;
        
        lmbdaptr callBack;
    private:
        void close();
        lmbdaptr streamClose;
    };
    
    using Headers = std::vector<std::pair<std::string, std::string>>;
    using UniqueResponse = std::unique_ptr<bcl::Response>;
    using FutureResp = std::shared_ptr<bcl::Response>;
    using FutureResponse = std::future<bcl::FutureResp>;
    //    using FuturePromise = std::promise<bcl::Response>;
    using MemoryByte = std::basic_string<unsigned char>;
    
    struct Request {
        CURL *curl;
        char errorBuffer[CURL_ERROR_SIZE];
        bcl::Args args;
        
        void* dataPtr; // For function call back data pointer
        void* cbPtr;
        Headers headers;
        
        lmbdaptr& attach_lmbdaptr();
        
        Request(bcl::Args &_args) {
            dataPtr = NULL;
            args = _args;
        }
        
        ~Request() {
        }
        lmbdaptr callBack;
    };
    
    void setArgs(bcl::Arg *args, int arg);
    void setArgs(bcl::Arg *args, float arg);
    void setArgs(bcl::Arg *args, long arg);
    void setArgs(bcl::Arg *args, char arg);
    void setArgs(bcl::Arg *args, const char* arg);
    
    template <typename ArgType, typename... ArgTypes>
    void setArgs(bcl::Arg *args, ArgType&& argType, ArgTypes&&... argTypes) {
        setArgs(args++, BACKCURL_FWD(argType)); // Increase Arg after add
        setArgs(args, BACKCURL_FWD(argTypes)...);
    }
    
    template <typename ArgType>
    bcl::Args args(ArgType&& argType) {
        bcl::Arg* args = new Arg[1];
        setArgs(args, BACKCURL_FWD(argType));
        return args;
    }
    
    template <typename ArgType, typename... ArgTypes>
    bcl::Args args(ArgType&& argType,  ArgTypes&&... argTypes) {
        const long int n_args = sizeof...(ArgTypes) + 1; // add 1 for first argument
        bcl::Arg* args = new Arg[n_args];
        setArgs(args, BACKCURL_FWD(argType));
        setArgs(args+1, BACKCURL_FWD(argTypes)...); // skip one Arg after first arg
        return args;
    }
    // End Request Args
    
    // Curl Options
    template <typename OptType>
    void setOpts(bcl::Request *req, CURLoption curlOpt, OptType&& optType) {
        if (curl_easy_setopt(req->curl, curlOpt, BACKCURL_FWD(optType)) != CURLE_OK) {
            throw req->errorBuffer;
        };
    }
    
    template <typename OptType, typename... OptTypes>
    void setOpts(bcl::Request *req, CURLoption a, OptType&& optType, CURLoption b,  OptTypes&&... optTypes) {
        setOpts(req, a, optType);
        setOpts(req, b, BACKCURL_FWD(optTypes)...);
    }
    // End Curl Options
    
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
            MAIN_LOOP_CALLBACK = 0x1A, ASYNC_CALL = 0x1B, SYNC = 0x1C, ASYNC_FUTURE = 0x1D
        };
        
        extern std::deque<bcl::UniqueResponse> _mainLoopTasks;
        extern std::mutex _tasks_mutex;
        
        void wrapResponse(bcl::Request &r, bcl::Response &resp);
        
        void __execute__(bcl::Request &request, bcl::UniqueResponse response, bcl::internal::CALL_TYPE callType);
        void __execute__(bcl::Request &request, bcl::Response &response);
        
        template <typename DataType, typename ReqCB>
        bcl::FutureResp __execute__(ReqCB reqCallBack, bcl::Args reqArgs = 0) {
            bcl::Request req(reqArgs);
            DataType *dt = new DataType;
            // ReqCB *reqCBPTR = new ReqCB(reqCallBack);
            req.dataPtr = dt;
            req.cbPtr = &reqCallBack;//reqCBPTR;
            req.attach_lmbdaptr() = [](void *x) {
                bcl::Request *r = (bcl::Request*) x;
                ReqCB *lambdaX = static_cast<ReqCB*>(r->cbPtr);
                (*lambdaX)(r);
            };
            
            bcl::FutureResp resp(new bcl::Response(reqArgs));
            resp->setStreamClose([](void *x) {
                delete static_cast<DataType*>(((bcl::Response*) x)->__body);
            });
            __execute__(req, *resp);
            return resp;
        }
        
        template <typename DataType, typename ReqCB, typename RespCB>
        void __execute__(ReqCB reqCallBack, RespCB respCallback, bcl::internal::CALL_TYPE callType,
                         bcl::Args reqArgs = 0) {
            bcl::Request req(reqArgs);
            DataType *dt = new DataType;
            // ReqCB *reqCBPTR = new ReqCB(reqCallBack);
            req.dataPtr = dt;
            req.cbPtr = &reqCallBack;//reqCBPTR;
            req.attach_lmbdaptr() = [](void *x) {
                bcl::Request *r = (bcl::Request*) x;
                ReqCB *lambdaX = static_cast<ReqCB*>(r->cbPtr);
                (*lambdaX)(r);
            };
            
            UniqueResponse resp(new bcl::Response(reqArgs));
            RespCB *respCBPTR = new RespCB(respCallback); // Due to moving other resource to other thread when using runOnUI, so have to do this
            resp->cbPtr = respCBPTR;
            resp->attach_lmbdaptr() = [](void *x) {
                bcl::Response *r = (bcl::Response*) x;
                RespCB *lambdaX = static_cast<RespCB*>(r->cbPtr);
                (*lambdaX)(r);
            };
            resp->setStreamClose([](void *x) {
                bcl::Response *r = (bcl::Response*) x;
                delete static_cast<DataType*>(r->__body);
                delete static_cast<RespCB*>(r->cbPtr);
            });
            
            __execute__(req, std::move(resp), callType);
            // delete static_cast<ReqCB*>(req.cbPtr);
        }
    }
    
    size_t
    writeContentCallback(void *contents, size_t size, size_t nmemb, void *contentWrapper);
    
    size_t
    writeByteCallback(void *contents, size_t size, size_t nmemb, void *userp);
    
    
    template <typename DataType = std::string, typename ReqCB, typename RespCB>
    void execute(ReqCB requestCallback, RespCB responseCallback, bcl::Args args=0) {
        bcl::internal::__execute__<DataType>(std::move(requestCallback), std::move(responseCallback), internal::SYNC, std::move(args));
    }
    
    template <typename DataType = std::string, typename ReqCB, typename RespCB>
    void executeAsync(ReqCB requestCallback, RespCB responseCallback, bcl::Args args=0) {
        std::async(std::launch::async, [](ReqCB requestCallback, RespCB responseCallback, bcl::Args args) {
            bcl::internal::__execute__<DataType>(std::move(requestCallback), std::move(responseCallback), internal::ASYNC_CALL, std::move(args));
        }, std::move(requestCallback), std::move(responseCallback), std::move(args) );
    }

    template <typename DataType = std::string, typename ReqCB>
    auto execFuture(ReqCB requestCallback, bcl::Args args =0) -> bcl::FutureResponse {
        return std::async(std::launch::async, [](ReqCB requestCallback, bcl::Args args) {
            return bcl::internal::__execute__<DataType>(std::move(requestCallback), std::move(args));
        }, std::move(requestCallback), std::move(args) );
    }

    template <typename DataType = std::string, typename ReqCB, typename RespCB>
    void executeOnUI(ReqCB requestCallback, RespCB responseCallback, bcl::Args args = 0) {
        std::thread([](ReqCB requestCallback, RespCB responseCallback, bcl::Args args) {
            bcl::internal::__execute__<DataType>(std::move(requestCallback), std::move(responseCallback), internal::MAIN_LOOP_CALLBACK, std::move(args));
        }, std::move(requestCallback), std::move(responseCallback), std::move(args)).detach();
    }
}
#endif /* BackCurl_h */
