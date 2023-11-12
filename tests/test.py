import time
from io import BytesIO

from PIL import Image as PIL_Image
import cv2
import numpy as np
from CameraController import CameraController
from ImageProcessor import ImageProcessor
from arena_api.system import system

#
# class TestCamera:
#     camera_controller = None
#
#     @pytest.fixture
#     def setup(self):
#         self.camera_controller = CameraController()
#
#     def test_opencv(self):
#         device = self.camera_controller.device
#
#         with device.start_stream():
#             duration = 10  # Adjust this to the desired number of seconds
#
#             # Get the current time
#             start_time = time.time()
#
#             # Run the while loop until the desired duration is reached
#             while time.time() - start_time < duration:
#                 npndarray = self.camera_controller.get_npimage()
#                 cv2.namedWindow("Lucid", cv2.WINDOW_NORMAL)
#                 # print('Resized Dimensions : ', resized.shape)
#                 cv2.imshow('Lucid', npndarray)
#                 """
#                 Destroy the copied item to prevent memory leaks
#                 """
#
#                 """
#                 Break if esc key is pressed
#                 """
#                 key = cv2.waitKey(1)
#                 if key == 27:
#                     break
#             cv2.destroyAllWindows()
#
#     @pytest.fixture
#     def teardown(self):
#         self.camera_controller.cleanup()


class TestBenchmark:
    # def test_benchmark_png(self, benchmark):
    #     with open('/home/rrhan/arena/Camera-Feed/images/test2.npy', 'rb') as f:
    #         npndarray = np.load(f)
    #
    #     result, png = benchmark(ImageProcessor.to_png, npndarray)
    #     assert result
    #     # png_data = encoded_image.tobytes()
    #     # with open('../images/compression.png', 'wb') as file:
    #     #     file.write(png_data)

    def test_benchmark_jpg(self, benchmark):
        with open('./tests/data/test2.npy', 'rb') as f:
            npndarray = np.load(f)

        result, jpg = benchmark(ImageProcessor.to_jpg, npndarray)
        assert result
        # jpg_data = encoded_image.tobytes()
        # with open('../images/compression.jpg', 'wb') as file:
        #     file.write(jpg_data)


#
# with open('../images/test2.npy', 'rb') as f:
#     array = np.load(f)
#     success, encoded_image = cv2.imencode(".png", array)
#     # if not success:
#     #     print("Failed to encode the image")
#     # else:
#     #     png_data = encoded_image.tobytes()
#     # with open('../images/test1.png', 'wb') as file:
#     #     file.write(png_data)
#     png_array = PIL_Image.fromarray(array)
#     image_bytesio = BytesIO()
#     png_array.save(image_bytesio, format="PNG")
#
#     # Get the bytes of the PNG image
#     image_data = image_bytesio.getvalue()
#     with open('../images/test2.png', 'wb') as file2:
#         file2.write(image_data)
#     # cv2.imshow("My Image", array)
#     # cv2.waitKey(0)
#     # cv2.destroyAllWindows()
# print(time.time())
