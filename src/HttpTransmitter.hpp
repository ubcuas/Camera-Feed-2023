// Copyright 2024 UBC Uncrewed Aircraft Systems

#ifndef SRC_HTTPTRANSMITTER_HPP_
#define SRC_HTTPTRANSMITTER_HPP_

#include <curl/curl.h>

#include <cuchar>
#include <memory>
#include <string>
#include <vector>

#include <opencv2/opencv.hpp>
#include "Pipeline.hpp"


class HttpTransmitter {
 public:
  HttpTransmitter();
  ~HttpTransmitter();
  bool send_imgfile(const std::string& url,
                    const std::string& file_path,
                    int64_t timestamp);
  bool send_imen(const std::string& url,
                 std::unique_ptr<EncodedData> encoded);

 private:
  CURL* curl;
};

#endif  // SRC_HTTPTRANSMITTER_HPP_
