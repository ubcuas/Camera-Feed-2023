from fastapi import FastAPI, File, UploadFile
import uvicorn
app = FastAPI()


@app.get("/")
async def feed():
    return {"msg": "hello world"}


@app.post("/feed")
async def feed(file: UploadFile):
    return {"file_name": file.filename}


if __name__ == "__main__":
    uvicorn.run(app, host="0.0.0.0", port=8000)