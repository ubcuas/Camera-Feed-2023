#ifndef SRC_FAKECAMERA_HPP_
#define SRC_FAKECAMERA_HPP_

#include "ICamera.hpp"

class FakeCamera : public ICamera {
 private:
  int64_t _epoch;           /**< Epoch time for timestamping. */
  // bool _trigger_state = false; /**< Trigger state flag. */
 public:
  FakeCamera();   // Constructor
  ~FakeCamera();  // Destructor
  /**
   * @brief Sets the pixel format for the camera.
   * @param pixelformat A string specifying the pixel format.
   */
  void set_pixelformat(const std::string &pixelformat);

  /**
   * @brief Turns off auto expsure and sets the exposure time for the camera.
   * @param exposuretime A float of range [359.328, 151839.528] specifying the
   * exposure time in microseconds.
   */
  void set_exposuretime(float exposuretime);

  /**
   * @brief Sets the gain for the camera.
   * @param gain A float of range [0.0, 27.045771199653988] specifying the gain
   * value. Default gain in 0.0
   */
  void set_gain(float gain);

  // /**
  //  * @brief Enables or disables the manual trigger mode for the camera.
  //  * @param trigger_on A boolean where true enables and false disables it.
  //  */
  // void set_trigger(bool trigger_on);

  /**
   * @brief Starts the image stream from the camera.
   * @param num_buffers An integer specifying the number of buffers to use.
   * Default is 10.
   */
  void start_stream(int num_buffers = 10);

  /**
   * @brief Stops the image stream from the camera.
   */
  void stop_stream();

  /**
   * @brief Retrieves an image from the camera.
   * @param pImage A pointer to an Fake::IImage pointer to store the retrieved
   * image.
   * @param timestamp A pointer to a 64-bit integer to store the image
   * timestamp.
   * @return A boolean indicating success (true) or failure (false).
   */
  std::unique_ptr<ImageData> get_image();
};

#endif  // SRC_FAKECAMERA_HPP_
