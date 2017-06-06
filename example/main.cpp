
#include <stdio.h>
#include <backcurl/BackCurl.h>
#include <sys/stat.h>
#include <fcntl.h>
/************************************ Test Scope ******************************************/
void simpleGetOption(bcl::Request &req) {
    bcl::setOpts(req, CURLOPT_URL , "http://www.google.com",
                 CURLOPT_FOLLOWLOCATION, 1L,
                 CURLOPT_WRITEFUNCTION, &bcl::writeContentCallback,
                 CURLOPT_WRITEDATA, req.dataPtr,
                 CURLOPT_USERAGENT, "libcurl-agent/1.0",
                 CURLOPT_RANGE, "0-20000000"
                );
}

int countUI = 0;

void doSync() {
    bcl::execute<bcl::MemoryByte>([](bcl::Request & req) {
        bcl::setOpts(req, CURLOPT_URL , "http://wallpapercave.com/wp/LmOgKXz.jpg",
                     CURLOPT_FOLLOWLOCATION, 1L,
                     CURLOPT_WRITEFUNCTION, &bcl::writeByteCallback,
                     CURLOPT_WRITEDATA, req.dataPtr,
                     CURLOPT_USERAGENT, "libcurl-agent/1.0",
                     CURLOPT_RANGE, "0-20000000"
                    );
    }, [&](bcl::Response & resp) {
        printf("Downloaded content 0x%02hx\n", (const unsigned char*)resp.getBody<bcl::MemoryByte>()->c_str());
        printf("bcl::MemoryByte size is %ld\n", resp.getBody<bcl::MemoryByte>()->size());
        double dl;
        if (!curl_easy_getinfo(resp.curl, CURLINFO_CONTENT_LENGTH_DOWNLOAD, &dl)) {
            printf("Downloaded %.0f bytes\n", dl);
        }
        long headerSize;
        if (!curl_easy_getinfo(resp.curl, CURLINFO_HEADER_SIZE, &headerSize)) {
            printf("Downloaded header size %ld bytes\n", headerSize);
        }
        printf("You have a argument = %d\n", resp.args[0].getInt);
    }, bcl::args(1,2,3));

}


void doFuture() {
    bcl::FutureResponse frp;

    frp = bcl::execFuture<std::string>(simpleGetOption);
    bool uiRunning = true;
    while (uiRunning)  {
        if (bcl::hasRequestedAndReady(frp)) {
            bcl::Response r = frp.get();
            printf("The data content is = %s\n", r.getBody<std::string>()->c_str());
            printf("Got Http Status code = %ld\n", r.code);
            printf("Got Content Type = %s\n", r.contentType.c_str());
            printf("Total Time Consume = %f\n", r.totalTime);
            printf("has Error = %s\n", !r.error.empty() ? "Yes" : "No");

            // Exit App
            uiRunning = false;
        }
        printf("\r Future Sync ==%s ----%d", "Drawing Graphiccccc with count elapsed ", countUI++);

    }

    if (!bcl::isProcessing(frp)) printf("no data process now, no more coming data\n\n" );

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
        bcl::setOpts(req, CURLOPT_URL , req.args[0].getStr,
        CURLOPT_FOLLOWLOCATION, 1L,
        CURLOPT_WRITEFUNCTION, &bcl::writeContentCallback,
        CURLOPT_WRITEDATA, req.dataPtr,
        CURLOPT_USERAGENT, "libcurl-agent/1.0",
        CURLOPT_RANGE, "0-200000"
                    );
    },
    [&](bcl::Response & resp) {
        printf("On UI === %s\n", resp.getBody<std::string>()->c_str());
        printf("Done , stop gui running with count ui %d\n", countUI );
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        gui_running = false;
    }, bcl::args("http://www.google.com"));

    while (gui_running) {
        doGuiWork();
        doUpdate();
        std::this_thread::sleep_for(std::chrono::milliseconds(1000 / 16));
    }
}


// File image upload and download
struct MyFileAgent {
    FILE *fd;
    struct stat file_info;
};

size_t write_data(void *ptr, size_t size, size_t nmemb, void *userData) {
    MyFileAgent* fAgent = (MyFileAgent*)userData;
    size_t written = fwrite((FILE*)ptr, size, nmemb, fAgent->fd);
    return written;
}

void doFileDownload() {
    const char *url = "http://wallpapercave.com/wp/LmOgKXz.jpg";
    char outfilename[] = "husky_dog_wallpaper.jpeg";
    bcl::execute<MyFileAgent>([&](bcl::Request & req) -> void {
        MyFileAgent* fAgent = (MyFileAgent*)req.dataPtr;
        fAgent->fd = fopen(outfilename, "ab+");
        bcl::setOpts(req, CURLOPT_URL, url,
        CURLOPT_WRITEFUNCTION, write_data,
        CURLOPT_WRITEDATA, req.dataPtr,
        CURLOPT_FOLLOWLOCATION, 1L
                    );
    }, [&](bcl::Response & resp) {
        printf("Response content type = %s\n", resp.contentType.c_str());
        fclose(resp.getBody<MyFileAgent>()->fd);
    });


}




// void doFileUpload() {
//     double speed_upload, total_time;
//     bcl::executeAsync<MyFileAgent>([&file_info](bcl::Request & req) -> void {
//          MyFileAgent* fAgent = (MyFileAgent*)req.dataPtr;
//          fAgent->fd = fopen("debugit", "rb"); /* open file to upload */
//         if (!fAgent->fd)
//             return; /* can't continue */

//         /* to get the file size */
//         if (fstat(fileno(fAgent->fd), &fAgent->file_info) != 0)
//             return; /* can't continue */

//         bcl::setOpts(req, CURLOPT_URL , "http://www.dropbox.com/....",
//         CURLOPT_UPLOAD, 1L
//         CURLOPT_READDATA, req.dataPtr,
//         CURLOPT_INFILESIZE_LARGE, (curl_off_t)fAgent->file_info.st_size,
//         CURLOPT_VERBOSE, 1L
//                     );
//     }, [&speed_upload, &fd](bcl::Response & resp) -> void {
//         curl_easy_getinfo(resp.curl, CURLINFO_SPEED_UPLOAD, speed_upload);
//         fprintf(stderr, "Speed: %.3f bytes/sec during %.3f seconds\n",
//         speed_upload, resp.total_time);
//         fclose(resp.getBody<MyFileAgent>()->fd);
//     });
// }


// ** compile by g++ --std=c++11 main.cpp -lbackcurl -lcurl -pthread ** /

int main()
{
    bcl::init();

    // doSync();

    doFuture();

    doRunOnUI();

    // doFileDownload();
    bcl::cleanUp();

    return 1;
}