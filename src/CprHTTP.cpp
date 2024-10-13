#include "CprHTTP.h"

#include <cpr/cpr.h>

#include <iostream>
#include <vector>

bool CprHTTP::send_imen(std::string url, std::vector<unsigned char> *buffer,
                        int64_t timestamp) {
  // Construct the file name
  std::string filename = std::to_string(timestamp) + ".jpg";

  // Create a multipart form data object

  // Perform the POST request
  auto response = cpr::Post(
      cpr::Url{url},
      cpr::Multipart{
          {"image", cpr::Buffer{buffer->begin(), buffer->end(), filename}}});

  // Check if the request was successful
  if (response.status_code == 200) {
    std::cout << "Request successful.\n";
    return true;
  }
  std::cerr << "Error: " << response.status_code << " - "
            << response.error.message << "\n";
  return false;
}
