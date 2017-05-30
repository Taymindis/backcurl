# backcurl
C++, pure libcurl based with customized set_easy_opt for different kind of requests for Mobile, NON BLOCK UI SYNC http request.



```
cd build
cmake ..
cmake --build .
ctest -VV
sudo make install
```



```
int main() {
	bcl::init(); // init when using

    bcl::execute<std::string>([&](bcl::Request &req) {
    	bcl::setOpts(req, CURLOPT_URL , "http://www.google.com",
                 CURLOPT_FOLLOWLOCATION, 1L,
                 CURLOPT_WRITEFUNCTION, &bcl::writeMemoryCallback,
                 CURLOPT_WRITEDATA, req.dataPtr,
                 CURLOPT_USERAGENT, "libcurl-agent/1.0",
                 CURLOPT_RANGE, "0-200000"
                );
	}, [&](bcl::Response & resp) {
        std::string ret =  std::string(resp.getBody<std::string>()->c_str());
        printf("Sync === %s\n", ret.c_str());
    });


    bcl::cleanUp(); // clean up when no more using
}
```


### for future syn non blocking

```	
	bcl::FutureResponse frp;

    frp = bcl::execFuture<std::string>(simpleGetOption);
    while(bcl::hasRequestedButNotReady(frp)) { << it will not block until the data is ready
        printf("\r Future Sync ==%s ----%d", "Drawing Graphiccccc with count elapsed ", countUI++);
    }

    bcl::Response r = frp.get();
    printf("The data content is = %s\n", r.getBody<std::string>()->c_str());
    printf("Got Http Status code = %ld\n", r.code);
    printf("Got Error = %s\n", r.error.c_str());

    if(!bcl::hasDataProcess(frp)) printf("no data process now, no more coming data\n\n" );

```    



### for run on ui and call back to Main thread
```
//please see the example/main.cpp doRunOnUI method
```


## for different kind of data type request by default is std::string based
```
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

    bcl::execute<myType>([&](bcl::Request &req) {
    	bcl::setOpts(req, CURLOPT_URL , "http://www.google.com",
                 CURLOPT_FOLLOWLOCATION, 1L,
                 CURLOPT_WRITEFUNCTION, &myOwnCallBack,
                 CURLOPT_WRITEDATA, req.dataPtr,
                 CURLOPT_USERAGENT, "libcurl-agent/1.0",
                 CURLOPT_RANGE, "0-200000"
                );
	}, [&](bcl::Response & resp) {
        myType ret =  resp.getBody<myType>();
    });


    bcl::cleanUp(); // clean up when no more using
}
```