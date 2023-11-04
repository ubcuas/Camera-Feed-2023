import cProfile
import ctypes
import os  # os.getcwd()
import time
from pathlib import Path

import numpy as np  # pip install numpy
from PIL import Image as PIL_Image  # pip install Pillow
from arena_api.enums import PixelFormat

from arena_api.system import system
import socket
from io import BytesIO

# Server address and port
server_address = ('localhost', 12345)
"""
Save: BGR8 to PNG
	This example demonstrates saving the images retrieved
	by the camera in an easy to share PNG format using PIL.
	This includes configuring camera nodes, getting and converting
	the buffers into a numpy array, and saving the numpy array 
	using PIL as a PNG image.
"""


def create_devices_with_tries():
    '''
    This function waits for the user to connect a device before raising
        an exception
    '''

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
            print(f'Created {len(devices)} device(s)')
            return devices
    else:
        raise Exception(f'No device found! Please connect a device and run '
                        f'the example again.')


def example_entry_point():
    """
    demonstrates Save: BGR8 to PNG
     (1) Setup stream nodemap
     (2) Configure camera nodes to required dimensions
     (3) Start the stream and acquire buffers
     (4) Convert the buffer to numpy array that can be
          used by PIL
     (5) Convert numpy array to image using PIL
     (6) Save the image
    """
    # Create a UDP client socket
    client_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

    # Connect to the server
    client_socket.connect(server_address)

    # Create a device
    devices = create_devices_with_tries()
    device = devices[0]
    print(f'Device used in the example:\n\t{device}')

    # Get device stream nodemap
    tl_stream_nodemap = device.tl_stream_nodemap

    # Enable stream auto negotiate packet size
    tl_stream_nodemap['StreamAutoNegotiatePacketSize'].value = True

    # Enable stream packet resend
    tl_stream_nodemap['StreamPacketResendEnable'].value = True

    # Get/Set nodes -----------------------------------------------------------
    nodes = device.nodemap.get_node(['Width', 'Height', 'PixelFormat'])

    # Nodes
    print('Setting Width to its maximum value')
    nodes['Width'].value = nodes['Width'].max

    print('Setting Height to its maximum value')
    height = nodes['Height']
    height.value = height.max

    """
    Set pixel format to BGR8, most cameras should support this pixel format
    """
    pixel_format_name = PixelFormat.BGR8
    print(f'Setting Pixel Format to {pixel_format_name}')
    nodes['PixelFormat'].value = pixel_format_name

    # Grab and save an image buffer -------------------------------------------
    print('Starting stream')
    with device.start_stream(1):

        print('Grabbing an image buffer')
        # Optional args
        image_buffer = device.get_buffer()
        print(f' Width X Height = '
              f'{image_buffer.width} x {image_buffer.height}')

        """
        To save an image Pillow needs an array that is shaped to
        (height, width). In order to obtain such an array we use numpy
        library
        """
        print('Converting image buffer to a numpy array')

        """		
        Buffer.pdata is a (uint8, ctypes.c_ubyte)
        Buffer.data is a list of elements each represents one byte. Therefore
        for BGR8 each element represents a pixel.
        """
        num_channels = 3
        buffer_bytes_per_pixel = int(len(image_buffer.data) / (image_buffer.width * image_buffer.height))
        """
        Buffer data as cpointers can be accessed using buffer.pbytes
        """

        array = (ctypes.c_ubyte * num_channels * image_buffer.width * image_buffer.height).from_address(
            ctypes.addressof(image_buffer.pbytes))
        """
        Create a reshaped NumPy array to display using OpenCV
        """
        npndarray = np.ndarray(buffer=array, dtype=np.uint8,
                               shape=(image_buffer.height, image_buffer.width, buffer_bytes_per_pixel))

        """
        Method 2 (from Buffer.pdata)

        A more general way (not used in this simple example)

        Creates an already reshaped array to use directly with
        pillow.
        np.ctypeslib.as_array() detects that Buffer.pdata is (uint8, c_ubyte)
        type so it interprets each byte as an element.
        For 16Bit images Buffer.pdata must be cast to (uint16, c_ushort)
        using ctypes.cast(). After casting, np.ctypeslib.as_array() can
        interpret every two bytes as one array element (a pixel).

        Code:
        nparray_reshaped = np.ctypeslib.as_array( image_buffer.pdata,
        (image_buffer.height, image_buffer.width))
        """

        # Save image
        print('Saving image')
        png_array = PIL_Image.fromarray(npndarray)
        image_bytesio = BytesIO()
        png_array.save(image_bytesio, format="PNG")

        # Get the bytes of the PNG image
        image_data = image_bytesio.getvalue()

        # Send the image data in smaller chunks
        client_socket.sendall(image_data)

        print(f"Sent the image packets to {server_address}")

        device.requeue_buffer(image_buffer)

    # Clean up ---------------------------------------------------------------

    system.destroy_device()
    client_socket.close()

    print('Destroyed all created devices')


if __name__ == '__main__':
    print('\nWARNING:\nTHIS EXAMPLE MIGHT CHANGE THE DEVICE(S) SETTINGS!')
    print('\nExample started\n')
    cProfile.run('example_entry_point()')

    print('\nExample finished successfully')
