# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

cmake_minimum_required(VERSION 3.17.1)
project(gqlmapi VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 20)

find_package(PEGTL 3.2.0 CONFIG REQUIRED)
find_package(cppgraphqlgen 4.4.0 CONFIG REQUIRED)

if(VCPKG_TARGET_TRIPLET)
  message(STATUS "Using ${VCPKG_TARGET_TRIPLET} triplet with vcpkg")

  # Match the MSVC runtime if we're using VCPKG with static libraries.
  if(VCPKG_TARGET_TRIPLET MATCHES [[^.+-static$]])
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    message(STATUS "Using MSVC runtime static library")
  else()
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")
    message(STATUS "Using MSVC runtime DLL")
  endif()
endif()

option(BUILD_SHARED_LIBS "Build a Windows DLL instead of a static library" ON)

add_subdirectory(schema)
add_subdirectory(mapistub)
add_subdirectory(src)

include(CTest)
if(BUILD_TESTING)
  add_subdirectory(test)
endif()

# Build a NuGet package
set(CPACK_PACKAGE_VENDOR "GqlMAPI")
set(CPACK_PACKAGE_DESCRIPTION "C++ GraphQL Service Bindings for MAPI")
set(CPACK_PACKAGE_CHECKSUM "SHA256")
set(CPACK_GENERATOR "NuGet")
include(CPack)
