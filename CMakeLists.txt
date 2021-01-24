# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

cmake_minimum_required(VERSION 3.17.1)
project(gqlmapi VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 17)

find_package(PEGTL CONFIG REQUIRED)
find_package(cppgraphqlgen CONFIG REQUIRED)

add_subdirectory(schema)
add_subdirectory(src)

include(CTest)
if(BUILD_TESTING)
  add_subdirectory(test)
endif()
