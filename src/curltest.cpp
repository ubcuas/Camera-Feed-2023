#include <iostream>
#include <curl/curl.h>
#include <unistd.h>
#include <thread>
#include <vector>
#include <CLI/CLI.hpp>
#include <opencv2/opencv.hpp>

#include "HttpTransmitter.h"


// void image_sender(std::string url) {
//     HttpTransmitter http_transmitter(url);
//     for (int i = 0; i < 1; i++) {
//         (void) http_transmitter.send("asdf.txt", 1719296810737l);
//         std::cout << "sent\n";
//     }
// }

int main(int argc, char *argv[]) {
    // int runtime = 0;
    // int exposureTime = 0;
    // int gain = 0;
    std::string url;
    std::string filename;

    CLI::App app{"CLI Example"};

    // app.add_option("-r,--runtime", runtime, "Set runtime")->required();
    // app.add_option("-e,--exposure", exposureTime, "Set exposure time (ms)")->check(CLI::PositiveNumber);
    // app.add_option("-g,--gain", gain, "Set gain")->check(CLI::PositiveNumber);
    app.add_option("-u,--url", url, "Set URL");
    app.add_option("-f,--filename", filename, "Set file");


    CLI11_PARSE(app, argc, argv);

    std::cout << url << '\n';
    
    curl_global_init(CURL_GLOBAL_ALL);

    std::vector<int> compression_params;

    compression_params.push_back(cv::IMWRITE_JPEG_QUALITY);
    compression_params.push_back(100); // Change the quality value (0-100)

    cv::Mat img = cv::imread("1719296810737.jpg", cv::IMREAD_COLOR);
    std::vector<uchar> buf;
    cv::imencode(".jpg", img, buf, compression_params);
    

    HttpTransmitter http_transmitter;
    // (void) http_transmitter.send_imgfile(url, filename, 1719296810737l);
    http_transmitter.send_imen(url, buf, 1918183719895l);
    std::cout << "sent\n";
    
    // std::vector<std::thread> senders;

    // for (int i = 0; i < numSenders; i++) {
    //     senders.push_back(std::thread(image_sender, url));
    // }

    // for (std::thread& sender : senders) {
    //     sender.join();
    //     std::cout << "Sender joined\n";
    // }
    return 0;
}