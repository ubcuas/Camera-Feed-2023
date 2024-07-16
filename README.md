# Camera-Feed
A C++ program for the operation and integration a LUCID GigE Vision camera. 

## Features

* A C++ API for configuring and operating the camera
* Multithreaded image capture, saving, and HTTP file transer
* Command arguments for run length, exposure time, and gain


## Getting Started

### Dependencies
* ArenaSDK (https://thinklucid.com/downloads-hub/)
```
sudo apt update && sudo apt upgrade -y
sudo apt install cmake
sudo apt install libssl-dev
sudo apt install nghttp2
sudo apt install libopencv-dev
sudo apt install libcurl4-openssl-dev
```
## Build Instructions
After installing the prerequisites, run these commands to build the project.
```
cmake -S . -B build
cd build/
make -j$(nproc)
```
> ***Note:*** Use ``-DCMAKE_BUILD_TYPE=Debug`` for debug information.
Use ``-DCMAKE_BUILD_TYPE=Release`` for production.
### Python ArenaSDK

* Place the ArenaSDK wheel file in `wheel` directory.
```
pip install -r requirements.txt
```

## Authors

* **Richard Han** - [@rrhan0](https://github.com/rrhan0)