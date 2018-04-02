# backcurl
C++, pure libcurl based with customized set_easy_opt for different kind of requests for Mobile, NON BLOCK UI SYNC http request.


### Run All the tests and mobile development build, if you do not need mobile development build, please go for second option.
```
cd build
cmake ..
cmake --build .
ctest -VV
sudo make install
```

### Skip all the tests and mobile development build, just install backcurl core lib into system
```
cd backcurl-core
mkdir build
cd build
cmake ..
make
sudo make install
```


```cpp
int main() {
    bcl::init(); // init when using

    bcl::execute<std::string>([&](bcl::Request *req) {
        bcl::setOpts(req, CURLOPT_URL , "http://www.google.com",
                 CURLOPT_FOLLOWLOCATION, 1L,
                 CURLOPT_WRITEFUNCTION, &bcl::writeContentCallback,
                 CURLOPT_WRITEDATA, req->dataPtr,
                 CURLOPT_USERAGENT, "libcurl-agent/1.0",
                 CURLOPT_RANGE, "0-200000"
                );
    }, [&](bcl::Response * resp) {
        std::string ret =  std::string(resp->getBody<std::string>()->c_str());
        printf("Sync === %s\n", ret.c_str());
    });


    bcl::cleanUp(); // clean up when no more using
}
```


## For future non blocking sync
### this is different with runOnUI method, you need to hold the FutureResponse in your mainthread and do verify bcl::hasRequestedAndReady(frp) for every update until the result is read

```cpp
bcl::FutureResponse frp;

frp = bcl::execFuture<std::string>(simpleGetOption);
bool uiRunning = true;
while (uiRunning)  {
    if (bcl::hasRequestedAndReady(frp)) {
        bcl::FutureResp r = frp.get();
        printf("The data content is = %s\n", r->getBody<std::string>()->c_str());
        printf("Got Http Status code = %ld\n", r->code);
        printf("Got Content Type = %s\n", r->contentType.c_str());
        printf("Total Time Consume = %f\n", r->totalTime);
        printf("has Error = %s\n", !r->error.empty() ? "Yes" : "No");

        // Exit App
        uiRunning = false;
    }
    printf("\r Future Sync ==%s ----%d", "Drawing Graphiccccc with count elapsed ", countUI++);

}

if (!bcl::isProcessing(frp)) printf("no data process now, no more coming data\n\n" );
```



## For run on ui and call back to Main thread, this need to be done by put `bcl::LoopBackFire();` in your update scope
```cpp
// Derived from example/main.cpp
void doGuiWork() {
    printf("\r %s --- %d", "Drawing thousand Pieces of Color with count elapsed ", countUI++);
}

void doUpdate() {
    bcl::LoopBackFire();
}

void doRunOnUI () {
    bool gui_running = true;
    std::cout << "Game is running thread: ";

    bcl::executeOnUI<std::string>([](bcl::Request * req) -> void {
        bcl::setOpts(req, CURLOPT_URL , "http://www.google.com",
        CURLOPT_FOLLOWLOCATION, 1L,
        CURLOPT_WRITEFUNCTION, &bcl::writeContentCallback,
        CURLOPT_WRITEDATA, req->dataPtr,
        CURLOPT_USERAGENT, "libcurl-agent/1.0",
        CURLOPT_RANGE, "0-200000"
                    );
    }, [&](bcl::Response * resp) {
        printf("On UI === %s\n", resp->getBody<std::string>()->c_str());
        printf("Done , stop gui running with count ui %d\n", countUI );
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        gui_running = false;
    });

    while (gui_running) {
        doGuiWork();
        doUpdate();
        std::this_thread::sleep_for(std::chrono::milliseconds(1000 / 16));
    }
}
```


## For different kind of data type request,  by default is std::string based
```cpp
struct myType {
    std::string s;
    float f;
};

size_t myMemoryCallBack(void *contents, size_t size, size_t nmemb, void *contentWrapper) {
    size_t realsize = size * nmemb;
    myType *memBlock = (myType *)contentWrapper;

    // do your jobs

    return realsize;
}


int main() {
    bcl::init(); // init when using

    bcl::execute<myType>([&](bcl::Request *req) {
        bcl::setOpts(req, CURLOPT_URL , "http://www.google.com",
                 CURLOPT_FOLLOWLOCATION, 1L,
                 CURLOPT_WRITEFUNCTION, &myMemoryCallBack,
                 CURLOPT_WRITEDATA, req->dataPtr,
                 CURLOPT_USERAGENT, "libcurl-agent/1.0",
                 CURLOPT_RANGE, "0-200000"
                );
    }, [&](bcl::Response * resp) {
        myType* ret =  resp->getBody<myType>();
    });


    bcl::cleanUp(); // clean up when no more using
}
```


## Set Opts in 1 line to customized request
```cpp
bcl::setOpts(req, CURLOPT_URL , "http://www.google.com",
                 CURLOPT_FOLLOWLOCATION, 1L,
                 CURLOPT_WRITEFUNCTION, &myMemoryCallBack,
                 CURLOPT_WRITEDATA, req->dataPtr,
                 CURLOPT_USERAGENT, "libcurl-agent/1.0",
                 CURLOPT_RANGE, "0-200000"
                );
```
### for e.g, proxy request, proxy authenthication Get, Post, Head, Put, Basic Auth, please refer to libcurl for more details : https://curl.haxx.se/libcurl/c/curl_easy_setopt.html




## Set your http header in your request scope if needed, it is depended on libcurl CURLOPT_HTTPHEADER.
```cpp
bcl::execute<myType>([&](bcl::Request *req) {
        req->headers->emplace_back("Authorization", res::mySetting[MY_BASIC_AUTH].asString());

        bcl::setOpts(req, CURLOPT_URL , "http://www.google.com",
                 CURLOPT_FOLLOWLOCATION, 1L,
                 CURLOPT_WRITEFUNCTION, &myMemoryCallBack,
                 CURLOPT_WRITEDATA, req->dataPtr,
                 CURLOPT_USERAGENT, "libcurl-agent/1.0",
                 CURLOPT_RANGE, "0-200000"
                );
    }, [&](bcl::Response * resp) {
        myType* ret =  resp->getBody<myType>();
    });
```


## Download image
### this needed to be done by wrapped file in the struct or class so it will be auto free the object. However, fd stream need to be closed at response scope
```cpp
struct MyFileAgent {
    FILE *fd;
    struct stat fileInfo;
};

size_t write_data(void *ptr, size_t size, size_t nmemb, void *userData) {
    MyFileAgent* fAgent = (MyFileAgent*)userData;
    size_t written = fwrite((FILE*)ptr, size, nmemb, fAgent->fd);
    return written;
}
const char *url = "http://wallpapercave.com/wp/LmOgKXz.jpg";
    char outfilename[] = "husky_dog_wallpaper.jpeg";
    bcl::execute<MyFileAgent>([&](bcl::Request * req) -> void {
        MyFileAgent* fAgent = (MyFileAgent*)req->dataPtr;
        fAgent->fd = fopen(outfilename, "ab+");
        bcl::setOpts(req, CURLOPT_URL,url,
        CURLOPT_WRITEFUNCTION, write_data,
        CURLOPT_WRITEDATA, req->dataPtr,
        CURLOPT_FOLLOWLOCATION, 1L
                    );
    }, [&](bcl::Response * resp) {
        printf("Response content type = %s\n", resp->contentType.c_str());
        fclose(resp->getBody<MyFileAgent>()->fd);
    });
```



## Get Image bytes into Memory
```cpp
bcl::execute<bcl::MemoryByte>([](bcl::Request * req) {
    bcl::setOpts(req, CURLOPT_URL , "http://wallpapercave.com/wp/LmOgKXz.jpg",
                 CURLOPT_FOLLOWLOCATION, 1L,
                 CURLOPT_WRITEFUNCTION, &bcl::writeByteCallback,
                 CURLOPT_WRITEDATA, req->dataPtr,
                 CURLOPT_USERAGENT, "libcurl-agent/1.0",
                 CURLOPT_RANGE, "0-20000000"
                );
}, [&](bcl::Response * resp) {
    printf("Downloaded content 0x%02hx\n", (const unsigned char*)resp->getBody<bcl::MemoryByte>()->c_str());
    printf("bcl::MemoryByte size is %ld\n", resp->getBody<bcl::MemoryByte>()->size());
    double dl;
    if (!curl_easy_getinfo(resp->curl, CURLINFO_CONTENT_LENGTH_DOWNLOAD, &dl)) {
        printf("Downloaded %.0f bytes\n", dl);
    }
    long headerSize;
    if (!curl_easy_getinfo(resp->curl, CURLINFO_HEADER_SIZE, &headerSize)) {
        printf("Downloaded header size %ld bytes\n", headerSize);
    }
});
```


## Using extra args for tracking information(alternative of lambda copy)
```cpp
for (std::vector<std::string>::iterator it =
     urls.begin();
     it != urls.end(); ++it) {
    std::string url = *it;
    static int count = 0;
    bcl::executeOnUI<bcl::MemoryByte>([](bcl::Request * req) -> void {
        log::messageln("Submitted Url is %s ", req->args[0].getStr);
        bcl::setOpts(req, CURLOPT_URL ,req->args[0].getStr,
                     CURLOPT_FOLLOWLOCATION, req->args[1].getLong,
                     CURLOPT_WRITEFUNCTION, &bcl::writeByteCallback,
                     CURLOPT_WRITEDATA, req->dataPtr,
                     CURLOPT_USERAGENT, req->args[2].getStr,
                     CURLOPT_RANGE, req->args[3].getStr
                     );
    }, [&](bcl::Response * resp) {
        int imageCount = resp->args[4].getInt;
        bcl::MemoryByte *byte = resp->getBody<bcl::MemoryByte>();
        log::messageln("Image byte size is %lu",byte->size());
        spSprite nvgSprite = new NVGImageSprite((unsigned char*) byte->c_str(), static_cast<int>(byte->size()),  3.0f, 0);
        nvgSprite->setPosition(60 * imageCount, 60 * imageCount);
        nvgSprite->setSize(getStage()->getSize()/5);
        //            nvgSprite->setPriority(200);
        log::messageln("The count is %d", imageCount);
        addChild(nvgSprite);
    }, bcl::args(url.c_str(), 1L, "libbackcurl-agent/1.0", "0-20000000", ++count));
}
```




