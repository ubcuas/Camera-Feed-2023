#include <iostream>
#include <curl/curl.h>

int main() {
    CURL *curl;
    CURLcode res;
    
    curl = curl_easy_init();

    if (curl == NULL) {
        fprintf(stderr, "HTTP request failed");
        return -1;
    }

    curl_easy_setopt(curl, CURLOPT_URL, "https://www.google.com");

    

    return 0;
}
