# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

find_package(GTest CONFIG REQUIRED)
include(GoogleTest)

add_library(testShared STATIC gmock_main.cpp MockObjects.cpp)
target_link_libraries(testShared PUBLIC gqlmapiCommon GTest::gtest GTest::gmock)

add_executable(schemaTest SchemaTest.cpp)
target_link_libraries(schemaTest PRIVATE testShared)
gtest_discover_tests(schemaTest)

add_executable(convertTest
  UnicodeTest.cpp
  DateTimeTest.cpp
  GuidTest.cpp)
target_link_libraries(convertTest PRIVATE testShared)
gtest_discover_tests(convertTest)
