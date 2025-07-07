// compute_extrinsic.hpp
#ifndef PROJECTION_HPP
#define PROJECTION_HPP

#include <opencv2/core.hpp>

/**
 * @brief Computes a rotation matrix from roll, pitch, and yaw angles in
 * degrees.
 *
 * @param roll_deg Roll angle in degrees (rotation around Y-axis)
 * @param pitch_deg Pitch angle in degrees (rotation around X-axis)
 * @param yaw_deg Yaw angle in degrees (rotation around Z-axis)
 * @return cv::Mat 3x3 rotation matrix of camera
 */
cv::Mat compute_extrinsic(double roll_deg, double pitch_deg, double yaw_deg);

/**
 * @brief Computes the X and Y offsets on the ground plane from camera
 * coordinates.
 *
 * @param altitude Altitude of the camera above the ground in meters
 * @param extrinsic 3x3 rotation matrix of camera
 * @param pixel_x X coordinate in image space
 * @param pixel_y Y coordinate in image space
 * @return cv::Point2d Offset (X, Y) in X meters East, Y meters North on the
 * ground plane
 */
cv::Point2d computeOffset(double altitude,
                          const cv::Mat& extrinsic,
                          double pixel_x,
                          double pixel_y);

/**
 * @brief Computes a new geographic position by offsetting from a base latitude
 * and longitude.
 *
 * @param lat Base latitude in degrees
 * @param lng Base longitude in degrees
 * @param x Offset in meters along the East direction
 * @param y Offset in meters along the North direction
 * @return std::pair<double, double> New latitude and longitude
 */
std::pair<double, double> computeGeoposition(double lat,
                                             double lng,
                                             double x,
                                             double y);

/**
 * @brief Converts image pixel coordinates to geographic coordinates using
 * camera parameters.
 *
 * @param roll_deg Roll angle in degrees (rotation around Y-axis)
 * @param pitch_deg Pitch angle in degrees (rotation around X-axis)
 * @param yaw_deg Yaw angle in degrees (rotation around Z-axis)
 * @param altitude Altitude of the camera above the ground in meters
 * @param pixel_x X coordinate in image space
 * @param pixel_y Y coordinate in image space
 * @param lat Base latitude in degrees
 * @param lng Base longitude in degrees
 * @return std::pair<double, double> Geographic coordinates (latitude,
 * longitude)
 */
std::pair<double, double> cam2Geoposition(double roll_deg,
                                          double pitch_deg,
                                          double yaw_deg,
                                          double altitude,
                                          double pixel_x,
                                          double pixel_y,
                                          double lat,
                                          double lng);

#endif  // PROJECTION_HPP
