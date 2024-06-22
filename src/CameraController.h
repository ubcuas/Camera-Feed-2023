#ifndef CAMERA_CONTROLLER_H
#define CAMERA_CONTROLLER_H

#include "ArenaApi.h"
#include "SaveApi.h"

/**
 * @class CameraController
 * @brief A class to control camera settings, streaming, and image saving.
 */
class CameraController {
public:
    /**
     * @brief Constructor for CameraController.
     * Initializes and configures the camera.
     */
    CameraController();

    /**
     * @brief Frees resources created by CameraController.
     */
    void cleanup();

    /**
     * @brief Sets the pixel format for the camera.
     * @param pixelformat A string specifying the pixel format.
     */
    void set_pixelformat(GenICam::gcstring pixelformat);

    /**
     * @brief Turns off auto expsure and sets the exposure time for the camera.
     * @param exposuretime A float of range [359.328, 151839.528] specifying the exposure time in microseconds.
     */
    void set_exposuretime(float exposuretime);

    /**
     * @brief Sets the gain for the camera.
     * @param gain A float of range [0.0, 27.045771199653988] specifying the gain value. Default gain in 0.0
     */
    void set_gain(float gain);

    /**
     * @brief Enables or disables the manual trigger mode for the camera.
     * @param trigger_on A boolean where true enables and false disables it.
     */
    void set_trigger(bool trigger_on);

    /**
     * @brief Sets the acquisition mode for the camera.
     * @param acq_mode A string specifying the acquisition mode [Continuous, SingleFrame, MultiFrame]. Continuous is default.
     */
    void set_acquisitionmode(GenICam::gcstring acq_mode);

    /**
     * @brief Starts the image stream from the camera.
     * @param num_buffers An integer specifying the number of buffers to use. Default is 10.
     */
    void start_stream(int num_buffers = 10);

    /**
     * @brief Stops the image stream from the camera.
     */
    void stop_stream();

    /**
     * @brief Retrieves an image from the camera.
     * @param pImage A pointer to an Arena::IImage pointer to store the retrieved image.
     * @param timestamp A pointer to a long to store the image timestamp.
     * @return A boolean indicating success (true) or failure (false).
     */
    bool get_image(Arena::IImage **pImage, long *timestamp);

    /**
     * @brief Saves an image to disk.
     * @param pImage A pointer to the image to save.
     * @param timestamp The UNIX timestamp in milliseconds associated with the image.
     * @return A string containing the file path of the saved image.
     */
    std::string save_image(Arena::IImage *pImage, long timestamp);
    
private:
    Arena::ISystem* pSystem;      /**< Pointer to the system object. */
    Arena::IDevice* pDevice;      /**< Pointer to the device object. */
    Save::ImageWriter writer;     /**< Image writer object for saving images. */
    int64_t epoch;                /**< Epoch time for timestamping. */
    bool trigger_state = false;   /**< Trigger state flag. */

    /**
     * @brief Sets default configurations for the camera.
     */
    void set_default();

    /**
     * @brief Configures the image writer.
     */
    void writer_config();

    /**
     * @brief Sets the epoch time.
     */
    void set_epoch();
};

#endif