import cv2

from camera_feed import CameraController

NODES = {
    'ExposureAuto': 'Off',
    'ExposureTime': float(1000),
    'Gain': float(27)
         }


def main():
    camera_controller = CameraController()

    with camera_controller.start_stream():
        # Run the while loop until the desired duration is reached
        while True:
            npndarray, timestamp = camera_controller.get_npimage()
            cv2.namedWindow("Lucid", cv2.WINDOW_NORMAL)
            # print('Resized Dimensions : ', resized.shape)
            cv2.imshow('Lucid', npndarray)
            """
            Destroy the copied item to prevent memory leaks
            """
            print(timestamp)
            """
            Break if esc key is pressed
            """
            key = cv2.waitKey(1)
            if key == 27:
                break
        cv2.destroyAllWindows()


if __name__ == '__main__':
    main()