import signal
import sys
import time

from camera_feed import CameraController

NODES = {
    'ExposureAuto': 'Off',
    'ExposureTime': float(1000),
    'Gain': float(27)
         }
camera_controller = CameraController()
def main():
    try:
        camera_controller.start_stream()
        prev_frame_time = 0
        for i in range(100):
            curr_frame_time = time.time()
            npndarray, timestamp = camera_controller.get_npimage()
            fps = str(1 / (curr_frame_time - prev_frame_time))
            print(timestamp)
            print(fps)
            prev_frame_time = curr_frame_time

        camera_controller.stop_stream()
    finally:
        camera_controller.cleanup()

# def cleanup():
#     camera_controller.cleanup()
#     sys.exit(0)
#
# signal.signal(signal.SIGINT, cleanup)
# signal.signal(signal.SIGTERM, cleanup)

if __name__ == '__main__':
    main()