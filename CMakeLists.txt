cmake_minimum_required(VERSION 3.16)
project(demo_sdl)

set(CMAKE_CXX_STANDARD 14)

# set out direcroty
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

set(FFMPEG_DEV_ROOT /opt/homebrew/Cellar/ffmpeg@4/4.4.2_4)

# set ffmpeg develop environment
include_directories(${FFMPEG_DEV_ROOT}/include)
link_directories(${FFMPEG_DEV_ROOT}/lib)
link_directories(${FFMPEG_DEV_ROOT}/lib64)

link_libraries(
    avcodec
    avformat
    avfilter
    avdevice
    swresample
    swscale
    avutil
)

find_package(SDL2 REQUIRED)
include_directories(demo_sdl ${SDL2_INCLUDE_DIRS})

include_directories(${CMAKE_SOURCE_DIR}/src)

add_executable(demo_sdl src/main.cpp)

target_link_libraries (demo_sdl ${SDL2_LIBRARIES})