#ifndef HTTP_TRANSMITTER_H
#define HTTP_TRANSMITTER_H

#include <string>
#include <curl/curl.h>
#include <vector>
#include <cuchar>


class HttpTransmitter {
public:
    HttpTransmitter();
    ~HttpTransmitter();
    bool send_imgfile(std::string url, std::string file_path, int64_t timestamp);
    bool send_imen(std::string url, std::vector<unsigned char> buffer, int64_t timestamp);


private:
    CURL *curl;
};

#endif