from fastapi import FastAPI
import uvicorn
from camera_feed import CameraController

app = FastAPI()
# camera_controller = CameraController()


@app.get("/")
def read_root():
    return {"Hello": "World"}


if __name__ == "__main__":
    uvicorn.run(app, host="0.0.0.0", port=8000)