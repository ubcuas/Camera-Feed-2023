# -----------------------------------------------------------------------------
# Copyright (c) 2022, Lucid Vision Labs, Inc.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
# EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
# OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
# NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
# BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
# ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
# CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.
# -----------------------------------------------------------------------------
import ctypes
import time

import numpy as np
from arena_api.system import system

from PIL import Image as PIL_Image
import socket
from io import BytesIO

'''
Trigger: Introduction
	This example introduces basic trigger configuration and use. In order to
	configure trigger, enable trigger mode and set the source and selector. The
	trigger must be armed before it is prepared to execute. Once the trigger is
	armed, execute the trigger and retrieve an image. 
'''
TAB1 = "  "
TAB2 = "    "

def create_devices_with_tries():
	'''
	Waits for the user to connect a device before
		raising an exception if it fails
	'''
	tries = 0
	tries_max = 6
	sleep_time_secs = 10
	devices = None
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
			return devices
	else:
		raise Exception(f'No device found! Please connect a device and run '
						f'the example again.')


def store_initial(nodemap):
	'''
	Stores intial node values, return their values at the end
	'''
	nodes = nodemap.get_node(['TriggerSelector', 'TriggerMode',
							  'TriggerSource', 'PixelFormat'])

	trigger_selector_initial = nodes['TriggerSelector'].value
	trigger_mode_initial = nodes['TriggerMode'].value
	trigger_source_initial = nodes['TriggerSource'].value
	pixel_format_initial = nodes['PixelFormat'].value

	initial_vals = [trigger_selector_initial, trigger_mode_initial,
					trigger_source_initial, pixel_format_initial]
	return nodes, initial_vals


def configure_trigger_acquire_image(device, nodemap, nodes, initial_vals, server_socket):
	'''
	demonstrates basic trigger configuration and use
	(1) sets pixel format, trigger mode, source, and selector
	(2) starts stream
	(3) waits until trigger is armed
	(4) triggers image
	(5) gets image
	(6) requeues buffer
	(7) stops stream
	'''

	setup(device, nodes)

	'''
	Start stream
		When trigger mode is off and the acquisition mode is set to stream
		continuously, starting the stream will have the camera begin acquiring a
		steady stream of images. However, with trigger mode enabled, the device
		will wait for the trigger before acquiring any.
	'''
	print(f'{TAB1}Start stream')
	device.start_stream()

	'''
	Trigger Armed
		Continually checks until trigger is armed. Once the trigger is armed, it
		is ready to be executed.
	'''
	print(f'{TAB2}Wait until trigger is armed')
	trigger_armed = False

	while trigger_armed is False:
		trigger_armed = bool(nodemap['TriggerArmed'].value)

	client_socket, client_address = server_socket.accept()
	print(f"Received connection from {client_address}")

	'''
	Trigger an image
		Trigger an image manually, since trigger mode is enabled. This triggers
		the camera to acquire a single image. A buffer is then filled and moved
		to the output queue, where it will wait to be retrieved.
	'''
	print(f'{TAB2}Trigger Image')
	nodemap['TriggerSoftware'].execute()

	'''
	Get image
		Once an image has been triggered, it can be retrieved. If no image has
		been triggered, trying to retrieve an image will hang for the duration of
		the timeout and then throw an exception.
	'''
	buffer = device.get_buffer()
	print(f"time: {buffer.timestamp_ns}")

	# Print some info about the image in the buffer
	print(f'{TAB2}Buffer received | ['
		  f'Width = {buffer.width} pxl, '
		  f'Height = {buffer.height} pxl]')
	num_channels = 3
	buffer_bytes_per_pixel = int(len(buffer.data) / (buffer.width * buffer.height))
	"""
	Buffer data as cpointers can be accessed using buffer.pbytes
	"""

	array = (ctypes.c_ubyte * num_channels * buffer.width * buffer.height).from_address(
		ctypes.addressof(buffer.pbytes))
	"""
	Create a reshaped NumPy array to display using OpenCV
	"""
	npndarray = np.ndarray(buffer=array, dtype=np.uint8,
						   shape=(buffer.height, buffer.width, buffer_bytes_per_pixel))

	send_image(client_socket, npndarray)

	# Requeue buffer
	print(f'{TAB2}Requeue Buffer')
	device.requeue_buffer(buffer)
	# elif data == 'stop':
	# 	# Stop stream
	print(f'{TAB1}Stop stream')
	device.stop_stream()

	'''
	Return nodes to their initial values
	'''
	nodes['TriggerSelector'].value = initial_vals[0]
	nodes['TriggerMode'].value = initial_vals[1]
	nodes['TriggerSource'].value = initial_vals[2]


def setup(device, nodes):
	nodes['PixelFormat'].value = 'BGR8'
	'''
	Set trigger selector
	Set the trigger selector to FrameStart. When triggered, the device will
	start acquiring a single frame. This can also be set to AcquisitionStart
	or FrameBurstStart.
	'''
	print(f'{TAB1}Set trigger selector to FrameStart')
	nodes['TriggerSelector'].value = 'FrameStart'
	'''
	Set trigger selector
	Set the trigger selector to FrameStart. When triggered, the device will
	start acquiring a single frame. This can also be set to AcquisitionStart
	or FrameBurstStart.
	'''
	print(f'{TAB1}Enable trigger mode')
	nodes['TriggerMode'].value = 'On'
	'''
	Set trigger mode
	Enable trigger mode before setting the source and selector and before
	starting the stream. Trigger mode cannot be turned on and off while the
	device is streaming.
	'''
	print(f'{TAB1}Set trigger source to software')
	nodes['TriggerSource'].value = 'Software'
	'''
	Setup stream values
	'''
	tl_stream_nodemap = device.tl_stream_nodemap
	tl_stream_nodemap['StreamAutoNegotiatePacketSize'].value = True
	tl_stream_nodemap['StreamPacketResendEnable'].value = True


# break


def send_image(client_socket, npndarray):
	# Save image
	print('Converting image to png')
	with open('../tests/data/test2.npy', 'wb') as f:
		np.save(f, npndarray)
	# success, encoded_image = cv2.imencode(".png", npndarray)
	# if not success:
	# 	print("Failed to encode the image")
	# else:
	# 	png_data = encoded_image.tobytes(
	# 	client_socket.sendall(png_data)
	# 	print(f"Sent the image packets to client")
	png_array = PIL_Image.fromarray(npndarray)
	image_bytesio = BytesIO()
	png_array.save(image_bytesio, format="PNG")

	# Get the bytes of the PNG image
	image_data = image_bytesio.getvalue()

	# Send the image data in smaller chunks
	client_socket.sendall(image_data)


def example_entry_point():
	# Create a UDP client socket

	server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
	server_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
	server_address = ('localhost', 12345)

	# Bind the socket to the server address
	server_socket.bind(server_address)

	# Listen for incoming connections
	server_socket.listen(1)

	print(f"Server is listening on {server_address}")


	devices = create_devices_with_tries()
	device = devices[0]

	nodemap = device.nodemap

	nodes, initial_vals = store_initial(nodemap)

	print("Example Started\n")
	configure_trigger_acquire_image(device, nodemap, nodes, initial_vals, server_socket)

	# Cleanup

	# Destory Device
	system.destroy_device(device)
	server_socket.close()

	print("\nExample Completed")


if __name__ == "__main__":
	example_entry_point()
