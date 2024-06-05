#include <iostream>
#include <curl/curl.h>
#include <thread>
#include <chrono>

#include "HttpTransmitter.h"

#define MAX_ATTEMPTS 100


HttpTransmitter::HttpTransmitter(std::string setURL) : url(setURL) {
    // curl_global_init(CURL_GLOBAL_ALL); Call once   
    // char errbuf[CURL_ERROR_SIZE];

    int attempts = 0;
    while ((curl = curl_easy_init()) == NULL && attempts < MAX_ATTEMPTS) {
        fprintf(stderr, "Failed to initialize curl handle, attempt %d of %d\n", attempts + 1, MAX_ATTEMPTS);
        attempts++;
    }

    if (curl == NULL) {
        fprintf(stderr, "Maximum attempts exceeded, exitting\n");
        exit(EXIT_FAILURE);
    }
    // Required for thread safety? https://curl.se/libcurl/c/threadsafe.html
    curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());

    // need sudo apt-get install -y nghttp2
    curl_easy_setopt(curl, CURLOPT_HTTP_VERSION,
                     (long)CURL_HTTP_VERSION_2_0);
    
    // curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, errbuf);

    // std::cout << "Destination endpoint: " << setURL << "\n";
}

HttpTransmitter::~HttpTransmitter() {
    curl_easy_cleanup(curl);
}

std::string HttpTransmitter::send(std::string image_path, long timestamp) {
    CURLcode res;
    
    curl_mime *form = NULL;
    curl_mimepart *field = NULL;

    int attempts = 0;
    while ((form = curl_mime_init(curl)) == NULL && attempts < MAX_ATTEMPTS) {
        fprintf(stderr, "Failed to initialize mime handle, attempt %d of %d\n", attempts + 1, MAX_ATTEMPTS);
        attempts++;
    }

    if (form) {
        /* Fill in the file upload field */
        attempts = 0;
        while ((field = curl_mime_addpart(form)) == NULL && attempts < MAX_ATTEMPTS) {
            fprintf(stderr, "Failed to initialize mime part, attempt %d of %d\n", attempts + 1, MAX_ATTEMPTS);
            attempts++;
        }
        curl_mime_name(field, "image");
        curl_mime_filedata(field, image_path.c_str());
    
        /* Fill in the filename field */
        std::string timestamp_str = std::to_string(timestamp);

        // Might be unnecessary, maybe include timestamp in name
        field = curl_mime_addpart(form);
        curl_mime_name(field, "timestamp");
        curl_mime_data(field, timestamp_str.c_str(), CURL_ZERO_TERMINATED);
    
        curl_easy_setopt(curl, CURLOPT_MIMEPOST, form);

        attempts = 0;
        while ((res = curl_easy_perform(curl)) != CURLE_OK && attempts < MAX_ATTEMPTS) {
            fprintf(stderr, "Failed to send request, attempt %d of %d: %s\n", attempts + 1, MAX_ATTEMPTS, curl_easy_strerror(res));
            attempts++;
            std::this_thread::sleep_for (std::chrono::seconds(1));
        }

        /* then cleanup the form */
        curl_mime_free(form);
    } else {
        fprintf(stderr, "Maximum attempts exceeded, skipping\n");

    }
    std::cout << "Success\n";

    return ""; // Placeholder response
}