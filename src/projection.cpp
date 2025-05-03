#include <opencv2/core.hpp>
#include <opencv2/calib3d.hpp>  // contains cv::undistortPoints

#include <cmath>
#include <GeographicLib/Geodesic.hpp>


#include "projection.hpp"

using namespace GeographicLib;

// Initialize constants
const double f_x = 2482.29888, f_y = 2485.09241;
const double c_x = 1358.69991, c_y = 904.861872;
const int img_w = 2736, img_h = 1824;

const double center_x = img_w / 2;
const double center_y = img_h / 2;

const cv::Mat intrinsic_matrix = (cv::Mat_<double>(3, 3) <<
    f_x, 0.0, c_x,
    0.0, f_y, c_y,
    0.0, 0.0, 1.0
);
cv::Mat distCoeffs = (cv::Mat_<double>(1, 5) << 
    -0.0528353529, -0.0938327045, -0.000127634646, -0.00201700049, 0.464714290);
/**
 * Computes a rotation matrix from roll, pitch, and yaw angles in degrees
 * 
 * @param roll_deg Roll angle in degrees (rotation around Y-axis)
 * @param pitch_deg Pitch angle in degrees (rotation around X-axis)
 * @param yaw_deg Yaw angle in degrees (rotation around Z-axis)
 * @return cv::Mat 3x3 rotation matrix
 */
cv::Mat compute_extrinsic(double roll_deg, double pitch_deg, double yaw_deg) {
  // Convert angles to radians
  const double DEG_TO_RAD = CV_PI / 180.0;
  double roll = roll_deg * DEG_TO_RAD;
  double pitch = pitch_deg * DEG_TO_RAD;
  double yaw = yaw_deg * DEG_TO_RAD;

  // Rotation matrix about X-axis
  cv::Mat Rx = (cv::Mat_<double>(3, 3) <<
      1, 0, 0,
      0, std::cos(pitch), -std::sin(pitch),
      0, std::sin(pitch), std::cos(pitch));

  // Rotation matrix about Y-axis
  cv::Mat Ry = (cv::Mat_<double>(3, 3) <<
      std::cos(roll), 0, std::sin(roll),
      0, 1, 0,
      -std::sin(roll), 0, std::cos(roll));

  // Rotation matrix about Z-axis
  cv::Mat Rz = (cv::Mat_<double>(3, 3) <<
      std::cos(yaw), -std::sin(yaw), 0,
      std::sin(yaw), std::cos(yaw), 0,
      0, 0, 1);

  // Combine rotation matrices: R = Rx * Ry * Rz
  cv::Mat R = Rx * Ry * Rz;
  
  return R;
}

cv::Point2d computeOffset(
    double altitude,
    const cv::Mat& extrinsic,
    double pixel_x,
    double pixel_y
) {
    double x = pixel_x;
    double y = img_h - pixel_y; // or flipped_y;

    // Prepare distorted point
    std::vector<cv::Point2d> distorted = { cv::Point2d(x, y) };
    std::vector<cv::Point2d> undistorted;

    // Use global or passed-in intrinsic and distortion coefficients
    cv::undistortPoints(distorted, undistorted, intrinsic_matrix, distCoeffs, cv::noArray(), intrinsic_matrix);

    // Convert to homogeneous coordinates
    cv::Mat pixel_hom = (cv::Mat_<double>(3, 1) << undistorted[0].x, undistorted[0].y, 1.0);

    // Compute world vector
    cv::Mat inv_intrinsic = intrinsic_matrix.inv();
    cv::Mat world_vec = extrinsic.t() * inv_intrinsic * pixel_hom;

    double scale = altitude / world_vec.at<double>(2, 0);
    cv::Mat offset = world_vec * scale;

    return cv::Point2d(offset.at<double>(0, 0), offset.at<double>(1, 0));
}

std::pair<double, double> computeGeoposition(double lat, double lng, double x, double y) {
    // don't use this near equator or prime meridian
    const Geodesic& geod = Geodesic::WGS84();

    double lat_x, lng_x, lat_xy, lng_xy;

    // Move east (90 degrees) by x meters
    geod.Direct(lat, lng, 90.0, x, lat_x, lng_x);

    // Move north (0 degrees) by y meters
    geod.Direct(lat_x, lng_x, 0.0, y, lat_xy, lng_xy);

    return {lat_xy, lng_xy};
}

std::pair<double, double> cam2Geoposition(double roll_deg, double pitch_deg, double yaw_deg, double altitude, double pixel_x, double pixel_y, double lat, double lng) {
    cv::Mat R = compute_extrinsic(roll_deg, pitch_deg, yaw_deg);
    cv::Point2d offset = computeOffset(altitude, R, pixel_x, pixel_y);
    std::pair<double, double> coords = computeGeoposition(lat, lng, offset.x, offset.y);
    return coords;
}