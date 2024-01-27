import socket 
import cv2
import time
import pickle
import struct
import imutils
import numpy as np
import time
import base64
from camera_feed import CameraController
from io import BytesIO
from PIL import Image
import ctypes
import datetime
import time
from collections import OrderedDict
from datetime import datetime, timezone, timedelta

import numpy as np

from camera_feed import ImageProcessor

HOST_IP = socket.gethostbyname(socket.gethostname())
'''
    alternatively we can dynamically get the host 
    HOST = '192.168.1.68' 
'''
PORT_TCP = 9090

BUFF_SIZE = 65536
PORT_UDP = 9999



    
def start_UDP_server(HOST, PORT, BUFF_SIZE): 
    

    
    server_socket = socket.socket(socket.AF_INET,socket.SOCK_DGRAM)
    server_socket.setsockopt(socket.SOL_SOCKET,socket.SO_RCVBUF,BUFF_SIZE)
    socket_address = (HOST,PORT)
    server_socket.bind(socket_address)
    print('Listening at:',socket_address)
    
    

def send_TCP(HOST, PORT):
    server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

    #UDP server = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    print(f"Host IP: {HOST}, PORT: {PORT}")
    # we send in a tuple containing host and port 
    server.bind((HOST,PORT))

    server.listen(5) # 5 means how many unaccepted connections we allow before we accept any new ones, so in this case its 5, not necessary always
    comm_socket, address = server.accept()  # the address is the address of client we connect to and comm_socket is what we use to talk to that client 
    try: 
        
        camera = CameraController()
        camera.start_stream()
        prev_frame_time = 0
        for i in range(100):
            curr_frame_time = time.time()
            npndarray, timestamp = camera.get_npimage()
            image_data = ImageProcessor.to_jpg_pil(npndarray)
            data = pickle.dumps(image_data)
            message_size = struct.pack("L", len(data))
                # Send the frame to the client
            comm_socket.sendall(message_size + data)
            fps = str(1 / (curr_frame_time - prev_frame_time))
            print(timestamp)
            print(fps)
            prev_frame_time = curr_frame_time

        camera.stop_stream()
    
    finally:
        camera.cleanup()





send_TCP(HOST = HOST_IP, PORT=PORT_TCP)