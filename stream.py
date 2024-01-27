import signal
import sys
import time
from datetime import datetime, timezone

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
        curr_frame_time = datetime.now(timezone.utc)
        for i in range(100):
            npndarray, timestamp = camera_controller.get_npimage()
            prev_frame_time = timestamp
            print(prev_frame_time - curr_frame_time)

            curr_frame_time = timestamp

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