import os
import shutil

from fastapi import FastAPI, File, UploadFile
from fastapi.responses import JSONResponse
import uvicorn


app = FastAPI()


@app.get("/")
async def feed():
    return {"msg": "hello world"}


@app.post("/image")
async def upload_file(image: UploadFile = File(...)):
    try:
        # change directory as preferred
        directory = "./imgs"
        os.makedirs(directory, exist_ok=True)

        # Save the file to a specified location
        with open(os.path.join(directory, image.filename), "wb") as buffer:
            shutil.copyfileobj(image.file, buffer)

        return JSONResponse(status_code=200, content={"message": "File uploaded successfully"})

    except Exception as e:
        return JSONResponse(status_code=500, content={"message": f"Could not upload file. Error: {str(e)}"})


if __name__ == "__main__":
    uvicorn.run(app, host="0.0.0.0", port=8000)