#include <iostream>
#include <cstring>

#include <curl/curl.h>
#include <thread>
#include <chrono>

#include "HttpTransmitter.h"

#define MAX_ATTEMPTS 100


struct WriteThis {
  const char *readptr;
  size_t sizeleft;
};


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
    // while ((form = curl_mime_init(curl)) == NULL && attempts < MAX_ATTEMPTS) {
    //     fprintf(stderr, "Failed to initialize mime handle, attempt %d of %d\n", attempts + 1, MAX_ATTEMPTS);
    //     attempts++;
    // }
    form = curl_mime_init(curl);
    if (form) {
        /* Fill in the file upload field */
        // attempts = 0;
        // while ((field = curl_mime_addpart(form)) == NULL && attempts < MAX_ATTEMPTS) {
        //     fprintf(stderr, "Failed to initialize mime part, attempt %d of %d\n", attempts + 1, MAX_ATTEMPTS);
        //     attempts++;
        // }
        field = curl_mime_addpart(form);
        curl_mime_name(field, "image");
        std::cout << image_path.c_str() << "\n";
        curl_mime_filedata(field, image_path.c_str());
    
        /* Fill in the filename field */
        std::string timestamp_str = std::to_string(timestamp);

        // Might be unnecessary, maybe include timestamp in name
        // field = curl_mime_addpart(form);
        // curl_mime_name(field, "timestamp");
        // curl_mime_data(field, timestamp_str.c_str(), CURL_ZERO_TERMINATED);
    
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
    std::cout << "Sent " << image_path << '\n';

    return ""; // Placeholder response
}

 
static size_t read_callback(char *dest, size_t size, size_t nmemb, void *userp)
{
  struct WriteThis *wt = (struct WriteThis *)userp;
  size_t buffer_size = size*nmemb;
 
  if(wt->sizeleft) {
    /* copy as much as possible from the source to the destination */
    size_t copy_this_much = wt->sizeleft;
    if(copy_this_much > buffer_size)
      copy_this_much = buffer_size;
    memcpy(dest, wt->readptr, copy_this_much);
 
    wt->readptr += copy_this_much;
    wt->sizeleft -= copy_this_much;
    return copy_this_much; /* we copied this many bytes */
  }
 
  return 0; /* no more data left to deliver */
}



std::string HttpTransmitter::send_imen(std::vector<unsigned char> buffer, long timestamp) {
    CURLcode res;
    

    int attempts = 0;
    struct WriteThis wt;
    const char* buf = (const char*)(buffer.data());
    wt.readptr = &buf[0];
    wt.sizeleft = buffer.size();
    std::string timestamp_str = std::to_string(timestamp);

    // std::string endpoint = url + "/" + timestamp_str.c_str();
    // std::cout << endpoint << "\n";
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    /* Now specify we want to POST data */
    curl_easy_setopt(curl, CURLOPT_POST, 1L);
    const char *data = "";
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data);

    // /* we want to use our own read function */
    // curl_easy_setopt(curl, CURLOPT_READFUNCTION, read_callback);

    // /* pointer to pass to our read function */
    // curl_easy_setopt(curl, CURLOPT_READDATA, &wt);

    /* get verbose debug output please */
    curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);

    /*
    If you use POST to an HTTP 1.1 server, you can send data without knowing
    the size before starting the POST if you use chunked encoding. You
    enable this by adding a header like "Transfer-Encoding: chunked" with
    CURLOPT_HTTPHEADER. With HTTP 1.0 or without chunked transfer, you must
    specify the size in the request.
    */

    /* Set the expected POST size. If you want to POST large amounts of data,
    consider CURLOPT_POSTFIELDSIZE_LARGE */
    // curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, (long)wt.sizeleft);

    while ((res = curl_easy_perform(curl)) != CURLE_OK && attempts < MAX_ATTEMPTS) {
        fprintf(stderr, "Failed to send request, attempt %d of %d: %s\n", attempts + 1, MAX_ATTEMPTS, curl_easy_strerror(res));
        attempts++;
        std::this_thread::sleep_for (std::chrono::seconds(1));
    }
    
    return ""; // Placeholder response
}