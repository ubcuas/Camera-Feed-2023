#ifndef CPRHTTP_H
#define CPRHTTP_H_H

#include <cpr/cpr.h>
#include <curl/curl.h>

#include <cuchar>
#include <string>
#include <vector>

class CprHTTP {
 public:
  bool send_imen(std::string url, std::vector<unsigned char> *buffer,
                 int64_t timestamp);
};

#endif