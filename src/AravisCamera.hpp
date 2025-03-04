#ifndef SRC_ARAVISCAMERA_HPP_
#define SRC_ARAVISCAMERA_HPP_

// #include "ArenaApi.h"
#include "ICamera.hpp"
#include <arv.h>

class AravisCamera : public ICamera {
 private:
  /**< Epoch time for timestamping. */
  // need to find member variables
  ArvCamera *arvCamera;
  GError *error = NULL;
  ArvStream *stream;
  ArvBuffer *arvBuffer;
  ArvGc *genicam;
  // bool _trigger_state = false; /**< Trigger state flag. */
 public:
  AravisCamera();   // Constructor
  ~AravisCamera();  // Destructor
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
   * Not sure about the parameters...
   */
  void start_stream();

  /**
   * @brief Stops the image stream from the camera.
   */
  void stop_stream();

  /**
   * @brief Retrieves an image from the camera.
   * @param timestamp A pointer to a 64-bit integer to store the image
   * timestamp.
   * @return A boolean indicating success (true) or failure (false).
   */
  std::unique_ptr<ImageData> get_image();

 private:
  /**
   * @brief Sets default configurations for the camera.
   */
  // void set_default();

  /**
   * @brief Sets the epoch time.
   * Is this still necessary if we can get the timestamp for an image?
   */
  void set_epoch();
};

#endif  // SRC_ARENACAMERA_HPP_
