cmake_minimum_required(VERSION 3.14)
project(exif_parser C)

set(CMAKE_C_STANDARD 11)
include_directories(include)

file(GLOB SRC_FILES
    src/*.c
)

add_library(exifparser STATIC ${SRC_FILES})
target_include_directories(exifparser PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
