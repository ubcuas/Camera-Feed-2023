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
    ${CMAKE_SOURCE_DIR}/src/Detector.cpp
    # ${CMAKE_SOURCE_DIR}/src/AravisCamera.cpp
    # ${CMAKE_SOURCE_DIR}/src/CprHTTP.cpp
    ${CMAKE_SOURCE_DIR}/src/Pipeline.cpp)


add_executable(curltest
    ${CMAKE_SOURCE_DIR}/src/curltest.cpp
    ${CMAKE_SOURCE_DIR}/src/HttpTransmitter.cpp
    ${CMAKE_SOURCE_DIR}/src/Detector.cpp
    )


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
set(WITH_OPENCL ON)

# find_package(PkgConfig REQUIRED)
# set(DEPENDENCIES "gobject-2.0" "glib-2.0" "aravis-0.8")
# pkg_check_modules(DEPS REQUIRED IMPORTED_TARGET ${DEPENDENCIES})


include(FetchContent)

FetchContent_Declare(
    json 
    URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.tar.xz
)
FetchContent_MakeAvailable(json)
# FetchContent_Declare(
#     asio 
#     GIT_REPOSITORY https://github.com/chriskohlhoff/asio.git
#     GIT_TAG        origin/master
# )
# set(ASIO_CMAKE_ASIO_TAG asio-1-30-2)

# FetchContent_MakeAvailable(asio)


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

# FetchContent_Declare(
#     cpr 
#     GIT_REPOSITORY https://github.com/libcpr/cpr.git
#     GIT_TAG 1.10.5
# ) # Replace with your desired git commit from: https://github.com/libcpr/cpr/releases
# FetchContent_MakeAvailable(cpr)

find_package(CURL REQUIRED) 

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
find_package(arena_sdk REQUIRED)


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
    nlohmann_json::nlohmann_json
    # asio
    # cpr::cpr
    # PkgConfig::DEPS
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

