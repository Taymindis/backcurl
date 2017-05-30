
#include <stdio.h>
#include <backcurl/BackCurl.h>
/************************************ Test Scope ******************************************/


void simpleGetOption(bcl::Request &req) {
    bcl::setOpts(req, CURLOPT_URL , "http://www.google.com",
                 CURLOPT_FOLLOWLOCATION, 1L,
                 CURLOPT_WRITEFUNCTION, &bcl::writeMemoryCallback,
                 CURLOPT_WRITEDATA, req.dataPtr,
                 CURLOPT_USERAGENT, "libcurl-agent/1.0",
                 CURLOPT_RANGE, "0-200000"
                );
}

int countUI = 0;

void doSync() {
    bcl::execute<std::string>(simpleGetOption, [&](bcl::Response & resp) {
        std::string ret =  std::string(resp.getBody<std::string>()->c_str());
        printf("Sync === %s\n", ret.c_str());
        //    return ret;
        // std::this_thread::sleep_for(std::chrono::milliseconds(500));
        return resp;
    });

}


void doFuture() {
    bcl::FutureResponse frp;

    frp = bcl::execFuture<std::string>(simpleGetOption);
    while(bcl::hasRequestedButNotReady(frp)) {
        printf("\r Future Sync ==%s ----%d", "Drawing Graphiccccc with count elapsed ", countUI++);
    }

    bcl::Response r = frp.get();
    printf("The data content is = %s\n", r.getBody<std::string>()->c_str());
    printf("Got Http Status code = %ld\n", r.code);
    printf("Got Error = %s\n", r.error.c_str());

    if(!bcl::hasDataProcess(frp)) printf("no data process now, no more coming data\n\n" );

}

void doGuiWork() {
    printf("\r %s --- %d", "Drawing thousand Pieces of Color with count elapsed ", countUI++);
    

}

void doUpdate() {
    bcl::LoopBackFire();
}

void doRunOnUI () {
    bool gui_running = true;
    std::cout << "Game is running thread: ";

    bcl::executeOnUI<std::string>([](bcl::Request & req) -> void {
        bcl::setOpts(req, CURLOPT_URL , "http://www.google.com",
        CURLOPT_FOLLOWLOCATION, 1L,
        CURLOPT_WRITEFUNCTION, &bcl::writeMemoryCallback,
        CURLOPT_WRITEDATA, req.dataPtr,
        CURLOPT_USERAGENT, "libcurl-agent/1.0",
        CURLOPT_RANGE, "0-200000"
                    );
    }, [&](bcl::Response & resp) {
        printf("On UI === %s\n", resp.getBody<std::string>()->c_str());
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


// ** compile by g++ --std=c++11 main.cpp -lbackcurl -lcurl -pthread ** /

int main()
{
    bcl::init();
    
    doSync();

    doFuture();

    doRunOnUI();


    bcl::cleanUp();

    return 1;
}