import socket
import cv2
import pickle
import struct



HOST = socket.gethostbyname(socket.gethostname())
 
'''
alternatively we can dynamically get the host 
HOST = socket.gethostbyname(socket.gethostname())
'''
# we dont wanna use 80 which is for http and 22 which i think is used for ssh 
PORT = 9090

# Client setup
client_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
client_socket.connect((HOST,PORT))

data = b""
payload_size = struct.calcsize("Q")
while True:
	print("Getting something")
	user_input = input("Enter something:")
	if user_input:
		break
client_socket.close()