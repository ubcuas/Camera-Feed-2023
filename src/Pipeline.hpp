#include <string>
#include <vector>

#include <opencv2/opencv.hpp>

#ifndef PIPELINE_HPP
#define PIPELINE_HPP

struct ImagePath {
  std::string path;
  int64_t timestamp;
};

struct EncodedData {
  std::vector<uchar> buf;
  int64_t timestamp;
};

struct DetectData {
  std::vector<cv::Point2d> points;
  int64_t timestamp;
  int32_t seq;
};

#endif  // PIPELINE_HPP
