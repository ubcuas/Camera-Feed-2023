// Copyright 2024 UBC Uncrewed Aircraft Systems

#ifndef ICAMERA_HPP
#define ICAMERA_HPP

#include <memory>
#include <opencv2/opencv.hpp>
#include <string>

struct ImageData {
  cv::Mat image;
  int64_t timestamp;
  int32_t seq;
};

/**
 * @class CameraController
 * @brief A class to control camera settings, streaming, and image saving.
 */
class ICamera {
 public:
  /// @brief Virtual destructor
  virtual ~ICamera() = default;

  /**
   * @brief Sets the pixel format for the camera.
   * @param pixelformat A string specifying the pixel format.
   */
  virtual void set_pixelformat(const std::string& pixelformat) = 0;

  /**
   * @brief Turns off auto expsure and sets the exposure time for the camera.
   * @param exposuretime A float of range [359.328, 151839.528] specifying the
   * exposure time in microseconds.
   */
  virtual void set_exposuretime(float exposuretime) = 0;

  /**
   * @brief Sets the gain for the camera.
   * @param gain A float of range [0.0, 27.045771199653988] specifying the gain
   * value. Default gain in 0.0
   */
  virtual void set_gain(float gain) = 0;

  /**
   * @brief Enables or disables the manual trigger mode for the camera.
   * @param trigger_on A boolean where true enables and false disables it.
   */
  virtual void enable_trigger(bool trigger_on) = 0;

  /**
   * @brief Starts the image stream from the camera.
   * @param num_buffers An integer specifying the number of buffers to use.
   * Default is 10.
   */
  virtual void start_stream(int num_buffers = 10) = 0;

  /**
   * @brief Stops the image stream from the camera.
   */
  virtual void stop_stream() = 0;

  /**
   * @brief Halves vertical and horizontal resolution
   */
  virtual void sensor_binning() = 0;

  /**
   * @brief Enables output pulse through wire on image capture
   */
  virtual void output_pulse() = 0;

  /**
   * @brief Retrieves an image from the camera.
   * @param pImage A pointer to an Arena::IImage pointer to store the retrieved
   * image.
   * @param timestamp A pointer to a 64-bit integer to store the image
   * timestamp.
   * @return A boolean indicating success (true) or failure (false).
   */
  virtual std::unique_ptr<ImageData> get_image(int timeout) = 0;

  // /**
  //  * @brief Saves an image to disk.
  //  * @param pImage A pointer to the image to save.
  //  * @param timestamp The UNIX timestamp in milliseconds associated with the
  //  image.
  //  * @return A string containing the file path of the saved image.
  //  */
  // std::string save_image(Arena::IImage *pImage, int64_t timestamp);

  /**
   * @brief Prints statistics of stream session.
   */
  // void get_statistics();

  //  protected:
  //   /**
  //    * @brief Sets default configurations for the camera.
  //    */
  //   virtual void set_default() = 0;

  //   /**
  //    * @brief Sets the epoch time.
  //    */
  //   virtual void set_epoch() = 0;

  //   /**
  //    * @brief Sets the acquisition mode for the camera.
  //    * @param acq_mode A string specifying the acquisition mode [Continuous,
  //    * SingleFrame, MultiFrame]. Continuous is default.
  //    */
  //   virtual void set_acquisitionmode(const std::string& acq_mode) = 0;
};

class timeout_exception : public std::runtime_error {
 public:
  using std::runtime_error::runtime_error;
};

#endif
