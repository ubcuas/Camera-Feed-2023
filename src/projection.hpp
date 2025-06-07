// compute_extrinsic.hpp
#ifndef PROJECTION_HPP
#define PROJECTION_HPP

#include <opencv2/core.hpp>

/**
 * Computes a rotation matrix from roll, pitch, and yaw angles in degrees.
 * 
 * @param roll_deg Roll angle in degrees (rotation around Y-axis)
 * @param pitch_deg Pitch angle in degrees (rotation around X-axis)
 * @param yaw_deg Yaw angle in degrees (rotation around Z-axis)
 * @return cv::Mat 3x3 rotation matrix
 */
cv::Mat compute_extrinsic(double roll_deg, double pitch_deg, double yaw_deg);

cv::Point2d computeOffset(
  double altitude,
  const cv::Mat& extrinsic,
  double pixel_x,
  double pixel_y
);

std::pair<double, double> computeGeoposition(double lat, double lng, double x, double y);

std::pair<double, double> cam2Geoposition(
  double roll_deg,
  double pitch_deg,
  double yaw_deg,
  double altitude,
  double pixel_x,
  double pixel_y,
  double lat,
  double lng
);

#endif // PROJECTION_HPP
