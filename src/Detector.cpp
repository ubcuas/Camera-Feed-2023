#include <opencv2/opencv.hpp>
#include <vector>

static cv::UMat kernel =
    cv::getStructuringElement(cv::MORPH_RECT, cv::Size(13, 13)).getUMat(cv::ACCESS_READ);

std::vector<cv::Point2d> predict_tophat(const cv::UMat& image) {
  cv::UMat whitehat, mask;

  // Apply White Hat transformation
  cv::morphologyEx(image, whitehat, cv::MORPH_TOPHAT, kernel);

  // Thresholding
  cv::threshold(whitehat, mask, 100, 255, cv::THRESH_BINARY);

  // Connected Components Analysis
  cv::Mat labels, stats, centroids;
  int num_labels =
      cv::connectedComponentsWithStats(mask, labels, stats, centroids);
  // Store detected object coordinates
  std::vector<cv::Point2d> object_centers;
  for (int i = 1; i < num_labels; ++i) {  // Start from 1 to ignore background
    double x = centroids.at<double>(i, 0);
    double y = centroids.at<double>(i, 1);
    object_centers.emplace_back(x, y);
  }

  return object_centers;
}