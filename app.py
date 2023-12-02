import atexit
import datetime
from io import BytesIO

import numpy as np
from fastapi import FastAPI, HTTPException, Request
import uvicorn
import base64
from camera_feed import CameraController, ImageProcessor

app = FastAPI()
camera_controller = CameraController()
camera_controller.start_stream()

atexit.register(camera_controller.cleanup)


@app.get("/")
def read_root():
    return {"Hello": "World"}


@app.get("/start")
def start_stream():
    try:
        camera_controller.start_stream()
    except Exception as e:
        print(e)
        return {'message': 'failure'}, 500
    return {'message': 'success'}


@app.get("/stop")
def stop_stream():
    try:
        camera_controller.stop_stream()
    except Exception as e:
        print(e)
        return {'message': 'failure'}, 500
    return {'message': 'success'}


@app.get("/image")
def take_image():
    try:
        npndarray, timestamp = camera_controller.get_npimage()
        image_data = base64.b64encode(ImageProcessor.to_jpg(npndarray))
    except Exception as e:
        print(e)
        return {'message': 'failure'}, 500
    return {'image': image_data,
            'timestamp': timestamp.isoformat()}


@app.post("/setup")
def take_image(request: Request):
    try:
        data = request.json()
        keyval = {v for k, v in data.__dict__.items()}
        camera_controller.setup(keyval)
    except Exception as e:
        print(e)
        return {'message': 'failure'}, 500
    return {'message': 'success'}


@app.get("/test")
def test_image():
    try:
        with open('./tests/data/test2.npy', 'rb') as f:
            npndarray = np.load(f)
        image_data = base64.b64encode(ImageProcessor.to_jpg(npndarray))
    except Exception as e:
        raise HTTPException(status_code=500, detail=e)
    return {'image': image_data,
            'timestamp': 1}


if __name__ == "__main__":
    uvicorn.run(app, host="0.0.0.0", port=8000)