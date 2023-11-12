import ctypes
import time
from collections import OrderedDict

import numpy as np
from arena_api.buffer import BufferFactory

from arena_api.system import system
DEFAULT_NODES = {
    'PixelFormat': 'BGR8'
}
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
    - setup(): Initialize and configure the camera.
    - stop_stream(): Begin capturing and streaming video.
    - end_stream(): Stop the video stream.
    - cleanup(): Release resources and perform cleanup tasks.
    """

    def __init__(self):
        self.device = None
        self.initial_vals = OrderedDict()
        self._create_devices_with_tries()
        self.setup(DEFAULT_NODES, DEFAULT_TL_STREAM_NODES)

    def setup(self, node_keyval, tl_stream_nodes_key_val):
        nodemap = self.device.nodemap
        tl_stream_nodemap = self.device.tl_stream_nodemap
        keys = node_keyval.keys()

        nodes = nodemap.get_node(keys)
        self._store_initial(nodes, keys)
        for key, val in node_keyval.items():
            nodes[key].value = val

        for key, val in tl_stream_nodes_key_val.items():
            tl_stream_nodemap[key].value = val

    def _create_devices_with_tries(self):
        """
        Waits for the user to connect a device before
            raising an exception if it fails
        """
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
        else:
            raise Exception(f'No device found! Please connect a device and run '
                            f'the example again.')

    def _store_initial(self, nodes, keys):
        '''
        Stores intial node values, return their values at the end
        '''
        for k in keys:
            if self.initial_vals.get(k) is None:
                self.initial_vals[k] = nodes.get(k).value

    def start_stream(self):
        self.device.start_stream()

    def stop_stream(self):
        self.device.stop_stream()

    def get_npimage(self):
        buffer_bytes_per_pixel = 3
        buffer = self.device.get_buffer()

        """
        Copy buffer and requeue to avoid running out of buffers
        """
        item = BufferFactory.copy(buffer)
        self.device.requeue_buffer(buffer)

        array = np.ndarray((ctypes.c_ubyte * buffer_bytes_per_pixel * item.width * item.height).from_address(
            ctypes.addressof(item.pbytes)))
        """
        Create a reshaped NumPy array to display using OpenCV
        """
        npndarray = np.ndarray(buffer=array, dtype=np.uint8,
                               shape=(item.height, item.width, buffer_bytes_per_pixel))

        return npndarray

    def cleanup(self):
        system.destroy_device(self.device)

    def __del__(self):
        self.cleanup()
