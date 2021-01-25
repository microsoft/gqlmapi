# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

cmake_minimum_required(VERSION 3.17.1)
project(mapistub)

set(CMAKE_CXX_STANDARD 17)

# Build MAPIStubLibrary as a static lib
add_library(mapistub STATIC
  ../library/mapiStubLibrary.cpp
  ../library/stubutils.cpp)
target_include_directories(mapistub
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../include
  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../library)
