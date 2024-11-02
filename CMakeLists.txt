cmake_minimum_required(VERSION 3.22.1)

# set(CMAKE_VERBOSE_MAKEFILE ON)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()


project(camerafeed)
set(CMAKE_CXX_STANDARD 20)


add_executable(${PROJECT_NAME} 
    ${CMAKE_SOURCE_DIR}/src/main.cpp
    ${CMAKE_SOURCE_DIR}/src/ArenaCamera.cpp
    ${CMAKE_SOURCE_DIR}/src/HttpTransmitter.cpp
    # ${CMAKE_SOURCE_DIR}/src/CprHTTP.cpp
    ${CMAKE_SOURCE_DIR}/src/Pipeline.cpp)


add_executable(curltest
    ${CMAKE_SOURCE_DIR}/src/curltest.cpp
    ${CMAKE_SOURCE_DIR}/src/HttpTransmitter.cpp)


add_compile_options(
    "-Wall" 
    "-Wextra" 
    "-Wpedantic" 
    "-Wcast-align" 
    "-Wcast-qual" 
    "-Wdisabled-optimization" 
    "-Wduplicated-branches"
    "-Wduplicated-cond"
    "-Wformat=2"
    "-Wlogical-op"
    "-Wmissing-include-dirs"
    # "-Wnull-dereference" # causes weird
    "-Woverloaded-virtual"
    "-Wpointer-arith"
    "-Wshadow"
    # "-Wswitch-enum"
    "-Wvla"
    "$<$<CONFIG:DEBUG>:-O0;-g3;-ggdb>"
    "$<$<CONFIG:RELEASE>:-O2>" # I don't know if O2 or O3 is better
    )
# install(TARGETS ${PROJECT_NAME} DESTINATION bin)
set(BUILD_LIST core,imgcodecs,highgui)
find_package(OpenCV REQUIRED)

include(FetchContent)

# FetchContent_Declare(
#   curl
#   GIT_REPOSITORY https://github.com/curl/curl.git
#   GIT_TAG         curl-8_5_0
# )
# FetchContent_MakeAvailable(curl)

FetchContent_Declare(
    cli11_proj
    QUIET
    GIT_REPOSITORY https://github.com/CLIUtils/CLI11.git
    GIT_TAG v2.4.2
)

FetchContent_MakeAvailable(cli11_proj)

FetchContent_Declare(
    cpr 
    GIT_REPOSITORY https://github.com/libcpr/cpr.git
    GIT_TAG 1.10.5
) # Replace with your desired git commit from: https://github.com/libcpr/cpr/releases
FetchContent_MakeAvailable(cpr)

find_package(CURL REQUIRED) 

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
find_package(arena_sdk REQUIRED)


#FetchContent_Declare(
#        opencv
#        GIT_REPOSITORY https://github.com/opencv/opencv.git
#        GIT_TAG        4.10.0
#)
#FetchContent_MakeAvailable(opencv)


target_include_directories(${PROJECT_NAME}  PRIVATE
    ${arena_sdk_INCLUDE_DIRS}
    ${OpenCV_INCLUDE_DIRS}
    ${CURL_INCLUDE_DIR}
)

target_link_libraries(${PROJECT_NAME}  PRIVATE
    ${arena_sdk_LIBRARIES}
    ${OpenCV_LIBS}
    ${CURL_LIBRARIES}
    CLI11::CLI11
    cpr::cpr
)

target_include_directories(curltest  PRIVATE
    ${OpenCV_INCLUDE_DIRS}
    ${CURL_INCLUDE_DIR}
)

target_link_libraries(curltest PRIVATE
    ${OpenCV_LIBS}
    ${CURL_LIBRARIES}
    CLI11::CLI11
)


# sudo apt-get install libssh2-1-dev

message("Current build type: ${CMAKE_BUILD_TYPE}\n")
# message("Current flags: ${CMAKE_CXX_FLAGS}\n")

