import cv2
import numpy as np
import requests
import base64

url = 'http://localhost:8000'


def main():
    # response = requests.get(f'{url}/start')
    # data = response.json()
    # print(data)
    while True:
        response = requests.get(f'{url}/test')
        data = response.json()
        image_data = data.get('image')
        image = base64.decode(image_data)
        npndarray = np.frombuffer(image, dtype=int)
        timestamp = data.get('timestamp')
        cv2.namedWindow("Lucid", cv2.WINDOW_NORMAL)
        # print('Resized Dimensions : ', resized.shape)
        cv2.imshow('Lucid', npndarray)
        """
        Destroy the copied item to prevent memory leaks
        """
        print(timestamp)
        """
        Break if esc key is pressed
        """
        key = cv2.waitKey(1)
        if key == 27:
            break
    cv2.destroyAllWindows()


if __name__ == '__main__':
    main()