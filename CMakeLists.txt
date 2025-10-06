# Copyright (c) 2016 The Chromium Embedded Framework Authors. All rights
# reserved. Use of this source code is governed by a BSD-style license that
# can be found in the LICENSE file.

# See the included README.md file for usage instructions.

# For VS2019 and Xcode 12+ support.
cmake_minimum_required(VERSION 3.19)
set(CMAKE_CXX_STANDARD 17)

# Only generate Debug and Release configuration types.
set(CMAKE_CONFIGURATION_TYPES Debug Release)

# Project name.
project(Test)

string(TOLOWER "${CMAKE_SYSTEM_NAME}" system_name_lower)
set(build_type $<IF:$<CONFIG:Debug>,Debug,$<IF:$<CONFIG:Release>,Release,Other>>)
set(OUTPUT_DIR_NAME bin_${system_name_lower}_${build_type})
set(OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/${OUTPUT_DIR_NAME}")
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /std:c++20")

SET( CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG "${OUTPUT_DIRECTORY}")
SET( CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE "${OUTPUT_DIRECTORY}")
SET( CMAKE_LIBRARY_OUTPUT_DIRECTORY_DEBUG "${OUTPUT_DIRECTORY}")
SET( CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELEASE "${OUTPUT_DIRECTORY}")
SET( CMAKE_ARCHIVE_OUTPUT_DIRECTORY_DEBUG "${OUTPUT_DIRECTORY}")
SET( CMAKE_ARCHIVE_OUTPUT_DIRECTORY_RELEASE "${OUTPUT_DIRECTORY}")

include(FetchContent)

include(cmake/TryDownload.cmake)
set(version_file version.h)
include(cmake/IncVersion.cmake)

FILE(GLOB_RECURSE  CSource src/*.c)
FILE(GLOB_RECURSE  CPPSource src/*.cpp)

message("CPPSource ${CPPSource}")

add_executable(test 
    glew/src/glew.c
    src/main.cpp
    ${CSource}
    ${CPPSource}
)

include_directories(
    glew/include
)

target_link_libraries(test
    opengl32
)