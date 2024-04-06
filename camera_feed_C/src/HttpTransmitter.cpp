#include <iostream>
#include <curl/curl.h>
#include "HttpTransmitter.h"

HttpTransmitter::HttpTransmitter(std::string setURL) : url(setURL) {
    std::cout << "Destination endpoint: " << setURL << "\n";
    curl_global_init(CURL_GLOBAL_ALL);

}

std::string HttpTransmitter::send(std::string image_path, long timestamp) {
    CURL *curl;
    CURLcode res;
    
    curl_mime *form = NULL;
    curl_mimepart *field = NULL;

    curl = curl_easy_init();
    if (curl) {
        /* Create the form */
        form = curl_mime_init(curl);
        if (form) {
            /* Fill in the file upload field */
            field = curl_mime_addpart(form);
            curl_mime_name(field, "image");
            curl_mime_filedata(field, image_path.c_str());
        
            /* Fill in the filename field */
            std::string timestamp_str = std::to_string(timestamp);

            field = curl_mime_addpart(form);
            curl_mime_name(field, "timestamp");
            curl_mime_data(field, timestamp_str.c_str(), CURL_ZERO_TERMINATED);
        
            /* what URL that receives this POST */
            curl_easy_setopt(curl, CURLOPT_URL, "https://5191dff8-f273-4a03-8fd2-b03e52934e9f.mock.pstmn.io/feed");
            curl_easy_setopt(curl, CURLOPT_MIMEPOST, form);

            // Requireed for thread safety? https://curl.se/libcurl/c/threadsafe.html
            curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);

        
            /* Perform the request, res gets the return code */
            res = curl_easy_perform(curl);
            /* Check for errors */
            // TODO retry if there is an error
            if(res != CURLE_OK)
            fprintf(stderr, "curl_easy_perform() failed: %s\n",
                    curl_easy_strerror(res));

            /* always cleanup */
            curl_easy_cleanup(curl);
        
            /* then cleanup the form */
            curl_mime_free(form);
        }
    }

    std::cout << "\n";
    return "HTTP response"; // Placeholder response
}