# cmake_minimum_required(VERSION 3.14)
# project(exif_parser C)

# set(CMAKE_C_STANDARD 11)
# include_directories(include)

# file(GLOB SRC_FILES
#     src/*.c
# )

# add_library(exifparser STATIC ${SRC_FILES})
# target_include_directories(exifparser PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)



cmake_minimum_required(VERSION 3.14)
project(exif_parser C)

set(CMAKE_C_STANDARD 11)
include_directories(include)

# Define VERBOSE as an optional CMake variable (default OFF)
option(VERBOSE "Enable verbose logging" OFF)

# If VERBOSE is ON or a value is passed, define it as a compiler macro
if(VERBOSE)
  add_compile_definitions(VERBOSE=1)
endif()

# Gather source files
file(GLOB SRC_FILES
    src/*.c
)

# Create static library
add_library(exifparser STATIC ${SRC_FILES})
target_include_directories(exifparser PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
