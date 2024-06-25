#include <iostream>
#include <curl/curl.h>
#include <unistd.h>
#include <thread>
#include <vector>
#include <CLI/CLI.hpp>

#include "HttpTransmitter.h"


void image_sender(std::string url) {
    HttpTransmitter http_transmitter(url);
    for (int i = 0; i < 1; i++) {
        (void) http_transmitter.send("asdf.txt", 1719296810737l);
        std::cout << "sent\n";
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
    
    curl_global_init(CURL_GLOBAL_ALL);

    HttpTransmitter http_transmitter(url);
    (void) http_transmitter.send("1719296810737.jpg", 1719296810737l);
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