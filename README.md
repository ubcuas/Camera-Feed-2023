# Camera-Feed
A C++ program for the operation and integration a LUCID GigE Vision camera. 

## Features

* A C++ API for configuring and operating the camera
* Multithreaded image capture, saving, and HTTP file transer
* Command arguments for run length, exposure time, and gain


## Getting Started

### Dependencies
* Cmake >= 3.22.1
* ArenaSDK (https://thinklucid.com/downloads-hub/)
```
sudo apt install libssl-dev
sudo apt install nghttp2
sudo apt install libopencv-dev
sudo apt install libcurl4-openssl-dev
```
### Python ArenaSDK

1. Download the `arena_api-2.3.3-py3-none-any.whl` wheel file if you haven't already.
2. Place the wheel file in your project's `wheel` directory.
3. Install packages

```
pip install -r requirements.txt
```
