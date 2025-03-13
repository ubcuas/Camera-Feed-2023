#include <opencv2/opencv.hpp>
#include <vector>

#ifndef DETECTOR_H
#define DETECTOR_H

std::vector<cv::Point2d> detect_tophat(const cv::UMat& image);


#endif