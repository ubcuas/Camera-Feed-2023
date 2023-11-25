import atexit
import datetime
from io import BytesIO

import numpy as np
from fastapi import FastAPI, HTTPException, Response
import uvicorn
import base64
from camera_feed import CameraController, ImageProcessor

app = FastAPI()
# camera_controller = CameraController()
# camera_controller.start_stream()
#
# atexit.register(camera_controller.cleanup)
#
#
# @app.get("/")
# def read_root():
#     return {"Hello": "World"}
#
#
# @app.get("/start")
# def start_stream():
#     try:
#         camera_controller.start_stream()
#     except Exception as e:
#         raise HTTPException(status_code=500, detail=e)
#     return {'message': 'success'}
#
#
# @app.get("/stop")
# def stop_stream():
#     try:
#         camera_controller.stop_stream()
#     except Exception as e:
#         raise HTTPException(status_code=500, detail=e)
#     return {'message': 'success'}
#
#
# @app.get("/image")
# def take_image():
#     try:
#         npndarray, timestamp = camera_controller.get_npimage()
#         result, image_data = ImageProcessor.to_jpg(npndarray)
#     except Exception as e:
#         raise HTTPException(status_code=500, detail=e)
#     return {'image': base64.b64encode(image_data.tobytes()),
#             'timestamp': timestamp.isoformat()}


@app.get("/test")
def take_image():
    try:
        with open('./tests/data/test2.npy', 'rb') as f:
            npndarray = np.load(f)
        image_data = ImageProcessor.to_jpg(npndarray).tobytes()
    except Exception as e:
        raise HTTPException(status_code=500, detail=e)
    return Response(content=image_data,
                    headers={'timestamp': datetime.datetime.utcnow().isoformat()},
                    media_type="image/jpeg")


if __name__ == "__main__":
    uvicorn.run(app, host="0.0.0.0", port=8000)