// Copyright 2024 UBC Uncrewed Aircraft Systems

#include "HttpTransmitter.hpp"

#include <curl/curl.h>
#include <string>
#include <cstdio>
#include <chrono>
#include <cstring>
#include <iostream>
#include <memory>
#include <thread>

#include <opencv2/opencv.hpp>

#include "Pipeline.hpp"

#define MAX_ATTEMPTS 100

// Callback function to handle the response
static size_t WriteCallback(void *contents,
                            size_t size,
                            size_t nmemb,
                            void *userp) {
  // Do nothing with the response
  return size * nmemb;
}

HttpTransmitter::HttpTransmitter() {
  // Verbose mode for debugging/devlopment
  // curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);

  int attempts = 0;
  while ((curl = curl_easy_init()) == NULL && attempts < MAX_ATTEMPTS) {
    fprintf(stderr,
            "curl_easy_init() failed, attempt %d of %d\n",
            attempts + 1,
            MAX_ATTEMPTS);
    attempts++;
  }

  if (curl == NULL) {
    fprintf(stderr, "Maximum attempts exceeded, exitting\n");
    exit(EXIT_FAILURE);
  }
  // Required for thread safety? https://curl.se/libcurl/c/threadsafe.html
  curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);

  // Support for HTTP2
  curl_easy_setopt(
      curl, CURLOPT_HTTP_VERSION, static_cast<int64_t>(CURL_HTTP_VERSION_2_0));

  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
}

HttpTransmitter::~HttpTransmitter() { curl_easy_cleanup(curl); }

bool HttpTransmitter::send_imgfile(const std::string &url,
                                   const std::string &file_path,
                                   int64_t timestamp) {
  CURLcode res;
  char error[CURL_ERROR_SIZE];
  curl_mime *form = NULL;
  curl_mimepart *field = NULL;
  std::string timestamp_str = std::to_string(timestamp);

  curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
  curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, error);

  form = curl_mime_init(curl);

  if (form == nullptr) {
    std::cout << "curl_mime_init() failed\n";
    return false;
  }
  
  curl_easy_setopt(curl, CURLOPT_MIMEPOST, form);
  field = curl_mime_addpart(form);
  if (field == nullptr) {
    std::cout << "curl_mime_addpart() failed\n";
    return false;
  }

  res = curl_mime_name(field, "image");
  if (res != CURLE_OK) {
    std::cout << "curl_mime_name() failed\n";
    return false;
  }

  res = curl_mime_filedata(field, file_path.c_str());  
  if (res != CURLE_OK) {
    std::cout << "curl_mime_filedata() failed: " << error << "\n";
    return false;
  }

  // TODO(Richard) Might be unnecessary to include timestamp since, I hope the
  // next server will be smarter than using the filename
  // Removed redundant timestamp_str definition

  // field = curl_mime_addpart(form);
  // curl_mime_name(field, "timestamp");
  // curl_mime_data(field, timestamp_str.c_str(), CURL_ZERO_TERMINATED);
  
  res = curl_easy_perform(curl);
  if (res != CURLE_OK) {
    std::cout << "curl_easy_perform() failed: " << error << "\n";
    return false;
  }

  /* then cleanup the form */
  curl_mime_free(form);

  std::cout << "Sent " << file_path << " to " << url << '\n';

  return true;
}

bool HttpTransmitter::send_imen(const std::string &url,
                                std::shared_ptr<EncodedData> encoded) {
  CURLcode res;
  char error[CURL_ERROR_SIZE];
  curl_mime *form = NULL;
  curl_mimepart *field = NULL;
  std::string timestamp_str = std::to_string(encoded->timestamp);
  std::string filename = timestamp_str + ".jpg";
  curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
  curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, error);

  form = curl_mime_init(curl);

  if (form == NULL) {
    std::cout << "curl_mime_init() failed\n";
    return false;
  }
  curl_easy_setopt(curl, CURLOPT_MIMEPOST, form);
  field = curl_mime_addpart(form);
  if (field == NULL) {
    std::cout << "curl_mime_addpart() failed\n";
    return false;
  }

  res = curl_mime_name(field, "image");
  if (res != CURLE_OK) {
    std::cout << "curl_mime_name() failed\n";
    return false;
  }
  res = curl_mime_data(field,
                       reinterpret_cast<const char *>(encoded->buf.data()),
                       encoded->buf.size());
  if (res != CURLE_OK) {
    std::cout << "curl_mime_data() failed\n";
    return false;
  }

  curl_mime_filename(field, filename.c_str());
  // while ((res = curl_mime_filedata(field, file_path.c_str())) != CURLE_OK &&
  // attempts < MAX_ATTEMPTS) {
  //     fprintf(stderr, "curl_mime_filedata() failed, attempt %d of %d: %s\n",
  //     attempts + 1, MAX_ATTEMPTS, error); attempts++;
  // }

  // TODO(Richard) Might be unnecessary to include timestamp since, I hope the
  // next server will be smarter than using the filename field =
  // curl_mime_addpart(form); curl_mime_name(field, "timestamp"); std::string
  // timestamp_str = std::to_string(timestamp); curl_mime_data(field,
  // timestamp_str.c_str(), CURL_ZERO_TERMINATED);

  res = curl_easy_perform(curl);
  if (res != CURLE_OK) {
    fprintf(stderr, "curl_mime_data() failed\n");
    return false;
  }

  /* then cleanup the form */
  curl_mime_free(form);

  std::cout << "Sent " << encoded->timestamp << " to " << url << '\n';

  return true;
}
