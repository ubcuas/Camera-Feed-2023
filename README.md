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

Init the submodules
```
git submodule init
git submodule update
```

Install pipx to prevent making virtual python environments
```
sudo apt-get install pipx
pipx ensurepath 
```

Now refresh the terminal after installing `pipx`


Install Conan
```
pipx install conan
conan profile detect --force
conan install . --outpu-folder=build --build=missing
```

Now go to the ArenaSDK download page and download the latest version of the SDK. (This is important to build the project)
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


## Hardware Setup
Camera hardware docs
https://support.thinklucid.com/phoenix-phx200s/ <br>
Flight controller camera config docs <br>
https://ardupilot.org/copter/docs/common-camera-shutter-with-servo.html#common-camera-shutter-with-servo-enhanced-camera-trigger-logging
1. Power the computer (outlet, RPi GPIO pins)
2. Connect ethernet cable from computer to camera
3. Plug in camera GPIO connector to camera
4. Plug in camera trigger wire into flight controller (Aux 6) (black to ground, brown line 2 to signal)
5. Plug in camera feedback wire into flight controller (Aux 5) (brown line 3 to signal)
6. Power camera (battery red wire, PoE)

## Authors
  
* **Richard Han** - [@rrhan0](https://github.com/rrhan0)