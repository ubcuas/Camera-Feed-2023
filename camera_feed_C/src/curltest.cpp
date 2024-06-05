#include <iostream>
#include <curl/curl.h>
#include <unistd.h>
#include <thread>
#include <vector>

#include "HttpTransmitter.h"

void image_sender(std::string url) {
    HttpTransmitter http_transmitter(url);
    for (int i = 0; i < 2; i++) {
        (void) http_transmitter.send("data/20240302_120654_836-image2.jpg");
    }
}

int main(int argc, char *argv[]) {
    std::string url;
    if (argc >= 2) {
        url = argv[1];
    }
    
    curl_global_init(CURL_GLOBAL_ALL);

    const int numSenders = 10;
    
    std::vector<std::thread> senders;

    for (int i = 0; i < numSenders; i++) {
        senders.push_back(std::thread(image_sender, url));
    }

    for (std::thread& sender : senders) {
        sender.join();
        std::cout << "Sender joined\n";
    }
    return 0;
}