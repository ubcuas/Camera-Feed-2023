#include <iostream>
#include <cstring>

#include <curl/curl.h>
#include <thread>
#include <chrono>

#include "HttpTransmitter.h"

#define MAX_ATTEMPTS 100


// struct WriteThis {
//   const char *readptr;
//   size_t sizeleft;
// };


HttpTransmitter::HttpTransmitter() {
    // Verbose mode for debugging/devlopment
    // curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);

    int attempts = 0;
    while ((curl = curl_easy_init()) == NULL && attempts < MAX_ATTEMPTS) {
        fprintf(stderr, "curl_easy_init() failed, attempt %d of %d\n", attempts + 1, MAX_ATTEMPTS);
        attempts++;
    }

    if (curl == NULL) {
        fprintf(stderr, "Maximum attempts exceeded, exitting\n");
        exit(EXIT_FAILURE);
    }
    // Required for thread safety? https://curl.se/libcurl/c/threadsafe.html
    curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);

    // Support for HTTP2
    curl_easy_setopt(curl, CURLOPT_HTTP_VERSION, (long)CURL_HTTP_VERSION_2_0);
}

HttpTransmitter::~HttpTransmitter() {
    curl_easy_cleanup(curl);
}

bool HttpTransmitter::send_imgfile(std::string url, std::string file_path, int64_t timestamp) {
    CURLcode res;
    char error[CURL_ERROR_SIZE];
    curl_mime *form = NULL;
    curl_mimepart *field = NULL;

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, error);

    int attempts = 0;
    while ((form = curl_mime_init(curl)) == NULL && attempts < MAX_ATTEMPTS) {
        fprintf(stderr, "curl_mime_init() failed, attempt %d of %d: %s\n", attempts + 1, MAX_ATTEMPTS, error);
        attempts++;
    }

    if (form == NULL) {
        fprintf(stderr, "Maximum attempts exceeded, skipping\n");
        return false;
    }
    curl_easy_setopt(curl, CURLOPT_MIMEPOST, form);

    attempts = 0;
    while ((field = curl_mime_addpart(form)) == NULL && attempts < MAX_ATTEMPTS) {
        fprintf(stderr, "curl_mime_addpart() failed, attempt %d of %d: %s\n", attempts + 1, MAX_ATTEMPTS, error);
        attempts++;
    }

    attempts = 0;
    while ((res = curl_mime_name(field, "image")) != CURLE_OK && attempts < MAX_ATTEMPTS) {
        fprintf(stderr, "curl_mime_name() failed, attempt %d of %d: %s\n", attempts + 1, MAX_ATTEMPTS, error);
        attempts++;
    }

    attempts = 0;
    while ((res = curl_mime_filedata(field, file_path.c_str())) != CURLE_OK && attempts < MAX_ATTEMPTS) {
        fprintf(stderr, "curl_mime_filedata() failed, attempt %d of %d: %s\n", attempts + 1, MAX_ATTEMPTS, error);
        attempts++;
    }


    // TODO Might be unnecessary to include timestamp since, I hope the next server will be smarter than using the filename
    std::string timestamp_str = std::to_string(timestamp);
    // field = curl_mime_addpart(form);
    // curl_mime_name(field, "timestamp");
    // curl_mime_data(field, timestamp_str.c_str(), CURL_ZERO_TERMINATED);


    attempts = 0;
    while ((res = curl_easy_perform(curl)) != CURLE_OK && attempts < MAX_ATTEMPTS) {
        fprintf(stderr, "curl_easy_perform() failed, attempt %d of %d: %s\n", attempts + 1, MAX_ATTEMPTS, error);
        attempts++;
        std::this_thread::sleep_for (std::chrono::seconds(1));
    }

    /* then cleanup the form */
    curl_mime_free(form);

    std::cout << "Sent " << file_path << " to " << url <<'\n';

    return true;
}

bool HttpTransmitter::send_imen(std::string url, std::vector<unsigned char> buffer, int64_t timestamp) {
    CURLcode res;
    char error[CURL_ERROR_SIZE];
    curl_mime *form = NULL;
    curl_mimepart *field = NULL;
    std::string timestamp_str = std::to_string(timestamp) + ".jpg";

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, error);

    int attempts = 0;
    while ((form = curl_mime_init(curl)) == NULL && attempts < MAX_ATTEMPTS) {
        fprintf(stderr, "curl_mime_init() failed, attempt %d of %d: %s\n", attempts + 1, MAX_ATTEMPTS, error);
        attempts++;
    }

    if (form == NULL) {
        fprintf(stderr, "Maximum attempts exceeded, skipping\n");
        return false;
    }
    curl_easy_setopt(curl, CURLOPT_MIMEPOST, form);

    attempts = 0;
    while ((field = curl_mime_addpart(form)) == NULL && attempts < MAX_ATTEMPTS) {
        fprintf(stderr, "curl_mime_addpart() failed, attempt %d of %d: %s\n", attempts + 1, MAX_ATTEMPTS, error);
        attempts++;
    }

    attempts = 0;
    while ((res = curl_mime_name(field, "image")) != CURLE_OK && attempts < MAX_ATTEMPTS) {
        fprintf(stderr, "curl_mime_name() failed, attempt %d of %d: %s\n", attempts + 1, MAX_ATTEMPTS, error);
        attempts++;
    }

    attempts = 0;
    while ((res = curl_mime_data(field, reinterpret_cast<const char*>(buffer.data()), buffer.size())) != CURLE_OK && attempts < MAX_ATTEMPTS) {
        fprintf(stderr, "curl_mime_filedata() failed, attempt %d of %d: %s\n", attempts + 1, MAX_ATTEMPTS, error);
        attempts++;
    }

    curl_mime_filename(field, timestamp_str.c_str());
    // while ((res = curl_mime_filedata(field, file_path.c_str())) != CURLE_OK && attempts < MAX_ATTEMPTS) {
    //     fprintf(stderr, "curl_mime_filedata() failed, attempt %d of %d: %s\n", attempts + 1, MAX_ATTEMPTS, error);
    //     attempts++;
    // }


    // TODO Might be unnecessary to include timestamp since, I hope the next server will be smarter than using the filename
    // field = curl_mime_addpart(form);
    // curl_mime_name(field, "timestamp");
    // std::string timestamp_str = std::to_string(timestamp);
    // curl_mime_data(field, timestamp_str.c_str(), CURL_ZERO_TERMINATED);


    attempts = 0;
    while ((res = curl_easy_perform(curl)) != CURLE_OK && attempts < MAX_ATTEMPTS) {
        fprintf(stderr, "curl_easy_perform() failed, attempt %d of %d: %s\n", attempts + 1, MAX_ATTEMPTS, error);
        attempts++;
        std::this_thread::sleep_for (std::chrono::seconds(1));
    }

    /* then cleanup the form */
    curl_mime_free(form);

    std::cout << "Sent " << timestamp << " to " << url <<'\n';

    return true;
}