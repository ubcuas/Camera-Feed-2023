cmake_minimum_required(VERSION 3.22.1)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

set(CMAKE_CXX_FLAGS "-Wall -Wextra -Wpedantic -Wcast-align -Wcast-qual -Wdisabled-optimization -Wduplicated-branches -Wduplicated-cond -Wformat=2 -Wlogical-op -Wmissing-include-dirs -Wnull-dereference -Woverloaded-virtual -Wpointer-arith -Wshadow -Wvla")
set(CMAKE_CXX_FLAGS_DEBUG "-g")
set(CMAKE_CXX_FLAGS_RELEASE "-O2")

project(camerafeed)
set(CMAKE_CXX_STANDARD 17)

add_executable(${PROJECT_NAME} 
    ${CMAKE_SOURCE_DIR}/src/main.cpp
    ${CMAKE_SOURCE_DIR}/src/CameraController.cpp
    ${CMAKE_SOURCE_DIR}/src/HttpTransmitter.cpp)

# add_executable(multithread
#     ${CMAKE_SOURCE_DIR}/src/multithreadtest.cpp)

add_executable(curltest
    ${CMAKE_SOURCE_DIR}/src/curltest.cpp
    ${CMAKE_SOURCE_DIR}/src/HttpTransmitter.cpp)

# install(TARGETS ${PROJECT_NAME} DESTINATION bin)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/")
find_package(arena_sdk REQUIRED)

message(arena_sdk_INCLUDE_DIRS="${arena_sdk_INCLUDE_DIRS}")
# target_include_directories(${PROJECT_NAME} PUBLIC 
#     ${CMAKE_SOURCE_DIR}/external/ArenaSDK_Linux_ARM64/include/Arena
#     ${CMAKE_SOURCE_DIR}/external/ArenaSDK_Linux_ARM64/include/GenTL
#     ${CMAKE_SOURCE_DIR}/external/ArenaSDK_Linux_ARM64/include/Save
#     ${CMAKE_SOURCE_DIR}/external/ArenaSDK_Linux_ARM64/GenICam/library/CPP/include
# )
include(FetchContent)

FetchContent_Declare(
  curl
  GIT_REPOSITORY https://github.com/curl/curl.git
  GIT_TAG         curl-8_5_0
)
FetchContent_MakeAvailable(curl)
# find_package(CURL REQUIRED)
# message(STATUS "CURL_INCLUDE_DIR: ${CURL_INCLUDE_DIR}")
# message(STATUS "CURL_LIBRARIES: ${CURL_LIBRARIES}")
target_include_directories(${PROJECT_NAME} PUBLIC 
    ${arena_sdk_INCLUDE_DIRS}
    ${CURL_INCLUDE_DIRS}
)

target_link_libraries(${PROJECT_NAME} PRIVATE 
    ${arena_sdk_LIBRARIES}
    CURL::libcurl
)

target_link_libraries(curltest PRIVATE 
    CURL::libcurl
)

# target_compile_options(${PROJECT_NAME} PUBLIC 
#     "-Wall" 
#     "-Wextra" 
#     "-Wpedantic" 
#     "-Wcast-align" 
#     "-Wcast-qual" 
#     "-Wdisabled-optimization" 
#     "-Wduplicated-branches"
#     "-Wduplicated-cond"
#     "-Wformat=2"
#     "-Wlogical-op"
#     "-Wmissing-include-dirs"
#     "-Wnull-dereference"
#     "-Woverloaded-virtual"
#     "-Wpointer-arith"
#     "-Wshadow"
#     # "-Wswitch-enum"
#     "-Wvla"
#     )
# sudo apt-get install libssh2-1-dev