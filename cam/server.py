import socket 
import cv2
import time
import pickle
import struct
import imutils


HOST = '192.168.1.68' 
'''
alternatively we can dynamically get the host 
HOST = socket.gethostbyname(socket.gethostname())
'''
# we dont wanna use 80 which is for http and 22 which i think is used for ssh 
PORT = 9090

server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

#UDP server = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
print(f"Host IP: {HOST}, PORT: {PORT}")
# we send in a tuple containing host and port 
server.bind((HOST,PORT))

server.listen(5) # 5 means how many unaccepted connections we allow before we accept any new ones, so in this case its 5, not necessary always






while True: 
        
            # WE DONT USE SERVER SOCKET TO TALK TO THE CLIENT!!!
        
        comm_socket, address = server.accept()  # the address is the address of client we connect to and comm_socket is what we use to talk to that client 
        print(f"Connected to Client with {address}")

        if comm_socket:
            capture = cv2.VideoCapture(0)
            capture.set(cv2.CAP_PROP_FRAME_WIDTH, 1920)
            capture.set(cv2.CAP_PROP_FRAME_HEIGHT, 1080)
            start_time = time.time()
            frame_count = 0
            while (capture.isOpened()):
                ret, frame = capture.read()

                # frame = imutils.resize(frame,width=320)

                data = pickle.dumps(frame)
                message_size = struct.pack("L", len(data))

                

                
                # Send the frame to the client
                comm_socket.sendall(message_size + data)

                frame_count += 1
                
                fps_send = frame_count / (time.time() - start_time)
                print(f"Sending FPS: {fps_send}")
                start_time = time.time()
                frame_count = 0


                            # Get the width and height of the frame
                                # Get the width and height of the captured video
                width = int(capture.get(cv2.CAP_PROP_FRAME_WIDTH))
                height = int(capture.get(cv2.CAP_PROP_FRAME_HEIGHT))

                # Display the width and height
                cv2.putText(frame, f"Width: {width}, Height: {height}", (10, 20),
                            cv2.FONT_HERSHEY_SIMPLEX, 0.5, (255, 255, 255), 1, cv2.LINE_AA)
                cv2.imshow("Transmitting Video", frame) 

                key = cv2.waitKey(1) & 0xFF

                if key == ord('q'):
                    comm_socket.close()
                    capture.release()
                    cv2.destroyAllWindows()
                    print(f"Conection with {address} ended!")




    

