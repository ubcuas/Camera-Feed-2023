#include <iostream>
#include <curl/curl.h>
#include <unistd.h>
#include <thread>
#include <vector>
#include <CLI/CLI.hpp>

#include "HttpTransmitter.h"


void image_sender(std::string url) {
    HttpTransmitter http_transmitter(url);
    for (int i = 0; i < 2; i++) {
        (void) http_transmitter.send("data/20240302_120654_836-image2.jpg", 1234567890l);
    }
}

int main(int argc, char *argv[]) {
    // int runtime = 0;
    // int exposureTime = 0;
    // int gain = 0;
    std::string url;

    CLI::App app{"CLI Example"};

    // app.add_option("-r,--runtime", runtime, "Set runtime")->required();
    // app.add_option("-e,--exposure", exposureTime, "Set exposure time (ms)")->check(CLI::PositiveNumber);
    // app.add_option("-g,--gain", gain, "Set gain")->check(CLI::PositiveNumber);
    app.add_option("-u,--url", url, "Set URL");

    CLI11_PARSE(app, argc, argv);

    std::cout << url << '\n';
    
    // curl_global_init(CURL_GLOBAL_ALL);

    // const int numSenders = 10;
    
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