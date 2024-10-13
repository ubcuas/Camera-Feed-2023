# Camera-Feed
A C++ program for the operation and integration a LUCID GigE Vision camera. 

## Features

* A C++ API for configuring and operating the camera
* Multithreaded image capture, saving, and HTTP file transer
* Command arguments for run length, exposure time, and gain


## Getting Started

### For Docker way

Ctrl + Shift + P -> Docker: Add Docker Files to Workspace (root) -> C++ -> Yes
Skip all the other parts

### Dependencies
* ArenaSDK (https://thinklucid.com/downloads-hub/)
```
sudo apt update && sudo apt upgrade -y
sudo apt install cmake libssl-dev nghttp2 libopencv-dev libcurl4-openssl-dev -y
```

Now go to the ArenaSDK download page and download the latest version of the SDK.
Extract the contents of the folder and put it into the external directory.

Follow the README for Arena SDK for Linux to install the SDK.
## Build Instructions
After installing the prerequisites, run these commands to build the project.
```
cmake -S . -B build
cd build/
make -j$(nproc)
```
> ***Note:*** Use ``-DCMAKE_BUILD_TYPE=Debug`` for debug information.
Use ``-DCMAKE_BUILD_TYPE=Release`` for production.

## Linter and Formatter
```
pip install cpplint
clang-format -i src/*.cpp src/*.hpp
cpplint --filter=-build/include_subdir src/*.cpp src/*.hpp
```

## Static Analysis
```
cppcheck src/*.cpp src/*.hpp
```


### Python ArenaSDK

* Place the ArenaSDK wheel file in `wheel` directory.
```
pip install -r requirements.txt
```

## Authors

* **Richard Han** - [@rrhan0](https://github.com/rrhan0)