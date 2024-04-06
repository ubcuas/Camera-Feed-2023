#ifndef HTTP_TRANSMITTER_H
#define HTTP_TRANSMITTER_H

#include <string>


class HttpTransmitter {
public:
    HttpTransmitter(std::string setURL);
    std::string send(std::string image_path, long timestamp);

private:
    std::string url;
};

#endif