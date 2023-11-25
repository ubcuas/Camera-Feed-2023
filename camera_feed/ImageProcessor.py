from io import BytesIO
from PIL import Image
import cv2


class ImageProcessor:
    @staticmethod
    def to_jpg(npndarray):
        encode_param = [cv2.IMWRITE_JPEG_QUALITY, 95]
        encimg = cv2.imencode('.jpg', npndarray, encode_param)[1].tostring()
        return encimg

    @staticmethod
    def to_jpg_pil(npndarray):
        img = Image.fromarray(npndarray)
        output_buffer = BytesIO()

        # Save the image to the buffer in JPEG format with the specified quality
        img.save(output_buffer, format="JPEG")

        # Get the compressed image data from the buffer
        compressed_data = output_buffer.getvalue()
        return compressed_data

    @staticmethod
    def to_png(npndarray):
        encode_param = [cv2.IMWRITE_PNG_COMPRESSION, 0]
        result, encimg = cv2.imencode('.png', npndarray, encode_param)
        return result, encimg

    @staticmethod
    def to_png_pil(npndarray):
        img = Image.fromarray(npndarray)

        output_buffer = BytesIO()

        # Save the image to the buffer in JPEG format with the specified quality

        img.save(output_buffer, format="PNG")

        # Get the compressed image data from the buffer
        compressed_data = output_buffer.getvalue()
        return compressed_data
