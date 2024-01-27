import cv2
import socket
import pickle
import struct

def receive_frame_chunks(client_socket):
    chunks = []
    chunk_size = 4096  # Adjust the chunk size to match the server
    while True:
        packet, _ = client_socket.recvfrom(65536)
        message_size = struct.unpack("L", packet[:8])[0]
        data = packet[8:]

        chunks.append(data)

        if len(data) < chunk_size:
            break

    return pickle.loads(b''.join(chunks))

# Client setup
client_socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

# Client IP address and port (update with the server's IP address)
server_address = ('192.168.1.68', 9999)
message = b'Hello'

client_socket.sendto(message, server_address)

while True:
    frame = receive_frame_chunks(client_socket)
    cv2.imshow("RECEIVING VIDEO", frame)

    key = cv2.waitKey(1) & 0xFF
    if key == ord('q'):
        client_socket.close()
        break
