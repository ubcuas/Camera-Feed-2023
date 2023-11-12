import socket
from pynput import keyboard

server_address = ('localhost', 12345)
image_file_name = './images/received_image.png'

# Create a socket and bind it to the server address
client_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
client_socket.connect(server_address)

with open(image_file_name, 'wb') as image_file:
    while True:
        data = client_socket.recv(1024)
        if not data:
            break
        image_file.write(data)
print(f"Image data received and saved as {image_file_name}")

client_socket.close()
