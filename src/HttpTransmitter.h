#ifndef HTTP_TRANSMITTER_H
#define HTTP_TRANSMITTER_H

#include <string>
#include <curl/curl.h>
#include <vector>
#include <cuchar>


class HttpTransmitter {
public:
    HttpTransmitter(std::string setURL);
    ~HttpTransmitter();
    std::string send(std::string image_path, long timestamp);
    std::string send_imen(std::vector<unsigned char> buffer, long timestamp);


private:
    std::string url;
    CURL *curl;
};

#endif