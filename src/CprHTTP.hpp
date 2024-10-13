// Copyright 2024 UBC Uncrewed Aircraft Systems

#ifndef SRC_CPRHTTP_HPP_
#define SRC_CPRHTTP_HPP_

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

#endif  // SRC_CPRHTTP_HPP_
