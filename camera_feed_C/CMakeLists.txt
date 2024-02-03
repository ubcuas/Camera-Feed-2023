cmake_minimum_required(VERSION 3.22.1)

# add_subdirectory(${CMAKE_SOURCE_DIR}/external/Are)


project(camerafeed)
set(CMAKE_CXX_STANDARD 17)

add_executable(${PROJECT_NAME} 
    ${CMAKE_SOURCE_DIR}/src/main.cpp
    ${CMAKE_SOURCE_DIR}/src/CameraController.cpp)

# install(TARGETS ${PROJECT_NAME} DESTINATION bin)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/")
find_package(arena_sdk REQUIRED)

target_include_directories(${PROJECT_NAME} PUBLIC ${arena_sdk_INCLUDE_DIRS})
message(arena_sdk_INCLUDE_DIRS="${arena_sdk_INCLUDE_DIRS}")
# target_include_directories(${PROJECT_NAME} PUBLIC 
#     ${CMAKE_SOURCE_DIR}/external/ArenaSDK_Linux_ARM64/include/Arena
#     ${CMAKE_SOURCE_DIR}/external/ArenaSDK_Linux_ARM64/include/GenTL
#     ${CMAKE_SOURCE_DIR}/external/ArenaSDK_Linux_ARM64/include/Save
#     ${CMAKE_SOURCE_DIR}/external/ArenaSDK_Linux_ARM64/GenICam/library/CPP/include
# )
target_link_libraries(${PROJECT_NAME} PRIVATE ${arena_sdk_LIBRARIES})

target_compile_options(${PROJECT_NAME} PUBLIC 
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
    "-Wnull-dereference"
    "-Woverloaded-virtual"
    "-Wpointer-arith"
    "-Wshadow"
    # "-Wswitch-enum"
    "-Wvla"
    )