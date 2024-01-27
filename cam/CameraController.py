import ctypes
import datetime
import time
from collections import OrderedDict
from datetime import datetime, timezone, timedelta

import numpy as np
from arena_api.buffer import BufferFactory

from arena_api.system import system
DEFAULT_NODES = [
    {'PixelFormat': 'BGR8'}
]
DEFAULT_TL_STREAM_NODES = {
    "StreamBufferHandlingMode": "NewestOnly",
    "StreamAutoNegotiatePacketSize": True,
    "StreamPacketResendEnable": True
}


class CameraController:
    """
    A class for controlling a camera, providing methods to set up, start, and end the camera stream,
    as well as cleanup resources.

    Methods:
    - setup(node_keyval): Initialize and configure the camera with specified node key-value pairs.
    - setup_tl(tl_stream_nodes_key_val): Configure the transport layer (TL) stream nodes of the camera.
    - start_stream(number_of_buffers): Begin capturing and streaming video with an optional number of buffers.
    - stop_stream(): Stop the video stream.
    - get_npimage(): Retrieve a NumPy array representing the latest captured image and its timestamp.
    - cleanup(): Release resources and perform cleanup tasks.
    """

    def __init__(self):
        """
        Constructor for CameraController class.
        Initializes the camera device, resets settings, and sets up default configurations.
        """
        self.device = None
        self._create_devices_with_tries()
        self._reset_settings()
        self.setup(DEFAULT_NODES)
        self.setup_tl(DEFAULT_TL_STREAM_NODES)
        self.reference_time = self._set_time()

    def setup(self, node_keyval):
        """
        Configures camera nodes given node_keyval with a list of dicts

        :param node_keyval: list of dicts containing key-value pairs [{key: value}, {key: value}]
        """
        print('Setting up device')
        nodemap = self.device.nodemap

        for dict in node_keyval:
            k, v = next(iter(dict.items()))
            nodemap.get_node(k).value = v

        for dict in node_keyval:
            nodemap.get_node(list(dict.keys())[0]).value = list(dict.values())[0]

    def setup_tl(self, tl_stream_nodes_key_val):
        tl_stream_nodemap = self.device.tl_stream_nodemap
        for key, val in tl_stream_nodes_key_val.items():
            tl_stream_nodemap[key].value = val

    def _set_time(self):
        """
        Set the reference time for camera timestamps
        :rtype: datetime: Timestamp representing the UTC reference time.
        """
        nodemap = self.device.nodemap
        timestamp_reset = nodemap.get_node('TimestampReset')
        timestamp_reset.execute()
        return datetime.now(timezone.utc)

    def _create_devices_with_tries(self):
        """
        Attempts to connect to camera before raising an exception if it fails
        """
        print('Connecting to device')
        tries = 0
        tries_max = 6
        sleep_time_secs = 10
        while tries < tries_max:  # Wait for device for 60 seconds
            devices = system.create_device()

            if not devices:
                print(
                    f'Try {tries + 1} of {tries_max}: waiting for {sleep_time_secs} '
                    f'secs for a device to be connected!')
                for sec_count in range(sleep_time_secs):
                    time.sleep(1)
                    print(f'{sec_count + 1} seconds passed ',
                          '.' * sec_count, end='\r')
                tries += 1
            else:
                self.device = devices[0]
                return
        else:
            raise Exception(f'No device found! Please connect a device and run '
                            f'the example again.')

    def start_stream(self, number_of_buffers=None):
        """
        Begin streaming with an optional number of buffers.

        :param number_of_buffers: Number of buffers to allocate for streaming (default=10)

        :return: Stream object representing the camera stream.
        """
        return self.device.start_stream(number_of_buffers)

    def stop_stream(self):
        """
        Stop the camera stream.
        """
        self.device.stop_stream()

    def get_npimage(self):
        """
       Retrieve a NumPy array representing the latest captured image and its timestamp.

       :return: Tuple containing:
          - np.ndarray: NumPy array representing the image.
          - datetime: Timestamp of the captured image.
        """
        buffer_bytes_per_pixel = 3
        buffer = self.device.get_buffer()

        """
        Copy buffer and requeue to avoid running out of buffers
        """
        item = BufferFactory.copy(buffer)
        self.device.requeue_buffer(buffer)

        array = (ctypes.c_ubyte * buffer_bytes_per_pixel * item.width * item.height).from_address(
            ctypes.addressof(item.pbytes))

        """
        Create a reshaped NumPy array to display using OpenCV
        """
        npndarray = np.ndarray(buffer=array, dtype=np.uint8,
                               shape=(item.height, item.width, buffer_bytes_per_pixel))
        timestamp = self.reference_time + timedelta(microseconds=item.timestamp_ns // 1000)

        """
        Destroy the copied item to prevent memory leaks
        """
        BufferFactory.destroy(item)

        return npndarray, timestamp

    def _reset_settings(self):
        """
        Reset device settings to default.
        """
        print('Resetting device')
        self.device.nodemap['UserSetSelector'].value = 'Default'
        self.device.nodemap['UserSetLoad'].execute()

    def cleanup(self):
        """
        Release camera and perform cleanup tasks.
        """
        print('Destroying device')
        system.destroy_device()