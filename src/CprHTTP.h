#ifndef CPRHTTP_H
#define CPRHTTP_H_H

#include <string>
#include <curl/curl.h>
#include <vector>
#include <cuchar>
#include <cpr/cpr.h>


class CprHTTP {
public:
    bool send_imen(std::string url, std::vector<unsigned char> *buffer, int64_t timestamp);
};

#endif