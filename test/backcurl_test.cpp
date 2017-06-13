#include <algorithm>

#include "BackCurl.h"
// #include "backcurl_test.h"
#include "gtest/gtest.h"


void simpleGetOption(bcl::Request *req) {
    bcl::setOpts(req, CURLOPT_URL , "http://www.google.com",
                 CURLOPT_FOLLOWLOCATION, 1L,
                 CURLOPT_WRITEFUNCTION, &bcl::writeContentCallback,
                 CURLOPT_WRITEDATA, req->dataPtr,
                 CURLOPT_USERAGENT, "libcurl-agent/1.0",
                 CURLOPT_RANGE, "0-200000"
                );
}

int countUI = 0;

void doSync() {
    bcl::execute<std::string>(simpleGetOption, [&](bcl::Response * resp) {
        std::string ret =  std::string(resp->getBody<std::string>()->c_str());
        printf("Sync === %s\n", ret.c_str());
        //    return ret;
        // std::this_thread::sleep_for(std::chrono::milliseconds(500));
        printf("%s\n", resp->args[0].getStr);
        printf("%ld\n", resp->args[1].getLong);
    },bcl::args("ABCD Args", 192321839L));

}


void doFuture() {
    bcl::FutureResponse frp;

    frp = bcl::execFuture<std::string>(simpleGetOption);
    while(bcl::hasRequestedButNotReady(frp)) {
        printf("\r Future Sync ==%s ----%d", "Drawing Graphiccccc with count elapsed ", countUI++);
    }

    bcl::FutureResp r = frp.get();
    printf("The data content is = %s\n", r->getBody<std::string>()->c_str());
    printf("Got Http Status code = %ld\n", r->code);
    printf("Got Error = %s\n", r->error.c_str());

    if(!bcl::isProcessing(frp)) printf("no data process now, no more coming data\n\n" );

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

    bcl::executeOnUI<std::string>([](bcl::Request * req) -> void {
        bcl::setOpts(req, CURLOPT_URL , req->args[0].getStr,
        CURLOPT_FOLLOWLOCATION, req->args[1].getLong,
        CURLOPT_WRITEFUNCTION, &bcl::writeContentCallback,
        CURLOPT_WRITEDATA, req->dataPtr,
        CURLOPT_USERAGENT, req->args[2].getStr,
        CURLOPT_RANGE, req->args[3].getStr
                    );
    },   
    [&](bcl::Response * resp) {
        printf("On UI === %s\n", resp->getBody<std::string>()->c_str());
        printf("Done , stop gui running with count ui %d\n", countUI );
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        gui_running = false;
    }, bcl::args("http://www.google.com", 1L, "libbackcurl-agent/1.0", "0-20000000"));

    while (gui_running) {
        doGuiWork();
        doUpdate();
        std::this_thread::sleep_for(std::chrono::milliseconds(1000 / 16));
    }
}




TEST(backcurl_test, sync_test)
{
   bcl::init();


   doSync();

   bcl::cleanUp();
}

TEST(backcurl_test, future_sync_test)
{
   bcl::init();


   doFuture();

   bcl::cleanUp();
}

TEST(backcurl_test, run_on_ui_test)
{
   bcl::init();


   doRunOnUI();

   bcl::cleanUp();
}

// TEST(cpp_sorter_test, char_arr_sort)
// {
//     char arr[] = {'a','b','c','d','e','f','g','h','a','b'};
//     char eq[]  = {'a','a','b','b','c','d','e','f','g','h'};
//     int sz = sizeof(arr)/sizeof(arr[0]);

//     array_sort(arr, sz);

//     for(int i=0; i<sz; i++)
// 	EXPECT_EQ(arr[i], eq[i+i]);
// }

// TEST(cpp_sorter_test, int_arr_sort)
// {
//     int arr[] = {9,8,7,6,5,4,3,2,1,0};
//     int eq[]  = {0,1,2,3,4,5,6,7,8,9};
//     int sz = sizeof(arr)/sizeof(arr[0]);

//     array_sort(arr, sz);

//     for(int i=0; i<sz; i++)
// 	EXPECT_EQ(arr[i], eq[i]);
// }
