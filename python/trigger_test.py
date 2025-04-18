from pymavlink import mavutil
import time

# Connect to the vehicle using the specified serial port
# connection_string = '/dev/tty.usbmodem1101'
connection_string = '127.0.0.1:14550'
vehicle = mavutil.mavlink_connection(connection_string, baud=57600)  # Default baud rate is 57600

# Wait for a heartbeat to establish connection
print("Waiting for heartbeat from device at /dev/tty.usbmodem1101...")
vehicle.wait_heartbeat()
print("Connected!")


def send_camera_trigger():
    """Send MAV_CMD_DO_DIGICAM_CONTROL command to trigger the camera"""
    # Command parameters for camera trigger
    vehicle.mav.command_long_send(
            0,
            0,
            mavutil.mavlink.MAV_CMD_DO_SET_CAM_TRIGG_DIST,
            0, 0, 1, 0, 0, 0, 0, 0
    )
    print("Camera trigger command sent")


def read_camera_feedback():
    """Listen for and print CAMERA_FEEDBACK messages"""
    print("Listening for camera feedback messages...")

    # Loop until we receive camera feedback or timeout
    start_time = time.time()
    timeout = 10  # 10 seconds timeout

    while True:
        msg = vehicle.recv_match(type='CAMERA_FEEDBACK', blocking=True)
        if msg:
            print("Received CAMERA_FEEDBACK message:")
            print(f"  Timestamp: {msg.time_usec}")
            print(f"  Camera ID: {msg.camera_id}")
            print(f"  Lat/Lon: {msg.lat / 1e7}, {msg.lng / 1e7}")
            print(f"  Altitude (m): {msg.alt_msl}")
            print(f"  Camera orientation (roll/pitch/yaw): {msg.roll}, {msg.pitch}, {msg.yaw}")
            print(f"  FOV: {msg.foc_len}")
            print(f"  Image index: {msg.img_idx}")
            print(f"  Capture result: {msg.flags}")
            return True
        print("Fail")


# Main execution
# send_camera_trigger()
# time.sleep(1)
read_camera_feedback()
vehicle.close()
# try:
#     # Trigger the camera
#     send_camera_trigger()
#
#     # Wait a moment for the camera to take the photo
#     time.sleep(1)
#
#     # Listen for camera feedback
#     read_camera_feedback()
#
# except Exception as e:
#     print(f"An error occurred: {e}")
#
# finally:
#     # Close connection
#     print("Closing connection")
#     vehicle.close()