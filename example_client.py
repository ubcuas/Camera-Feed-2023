import cv2
import numpy as np
import requests
import base64

url = 'http://localhost:8000'


def main():
    # response = requests.get(f'{url}/start')
    # print(response.json().get('message'))
    while True:
        response = requests.get(f'{url}/image')
        data = response.json()
        image_data = data.get('image')
        image = base64.b64decode(image_data)
        timestamp = data.get('timestamp')
        with open(f'./images/{timestamp}.jpg', 'wb') as out:
            out.write(image)

# def main():
#     response = requests.get(f'{url}/test')
#     data = response.json()
#     image_data = data.get('image')
#     image = base64.b64decode(image_data)
#     timestamp = data.get('timestamp')
#     print(timestamp)
#     with open('./images/b64test.jpg', 'wb') as out:
#         out.write(image)


if __name__ == '__main__':
    main()