import socket

# Configure the server
server_ip = 'localhost'
server_port = 12345
image_file_name = 'received_image.png'  # Name for the saved PNG file

# Create a socket and bind it to the server address
server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
server_socket.bind((server_ip, server_port))
server_socket.listen(1)  # Listen for incoming connections

print(f"Server is listening on {server_ip}:{server_port}")

# Accept a connection from a client
client_socket, client_address = server_socket.accept()
print(f"Accepted connection from {client_address}")

# Receive image data and save it to a PNG file
with open(image_file_name, 'wb') as image_file:
    while True:
        data = client_socket.recv(1024)
        if not data:
            break
        image_file.write(data)

print(f"Image data received and saved as {image_file_name}")

# Close the sockets
client_socket.close()
server_socket.close()
