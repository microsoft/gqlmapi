# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

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

find_package(GTest CONFIG REQUIRED)
include(GoogleTest)

add_library(testShared STATIC gmock_main.cpp MockObjects.cpp)
target_link_libraries(testShared PUBLIC gqlmapi GTest::gtest GTest::gmock)
target_include_directories(testShared PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../src)

add_executable(schemaTest SchemaTest.cpp)
target_link_libraries(schemaTest PRIVATE testShared)
gtest_discover_tests(schemaTest)

add_executable(convertTest
  UnicodeTest.cpp
  DateTimeTest.cpp
  GuidTest.cpp)
target_link_libraries(convertTest PRIVATE testShared)
gtest_discover_tests(convertTest)
