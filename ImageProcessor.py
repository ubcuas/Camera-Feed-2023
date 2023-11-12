import cv2


class ImageProcessor:
    @staticmethod
    def to_jpg(img):
        encode_param = [cv2.IMWRITE_JPEG_QUALITY, 95]
        result, encimg = cv2.imencode('.jpg', img, encode_param)
        return result, encimg

    # @staticmethod
    # def to_png(img):
    #     encode_param = [cv2.IMWRITE_PNG_COMPRESSION, 1]
    #     result, encimg = cv2.imencode('.png', img, encode_param)
    #     return result, encimg
