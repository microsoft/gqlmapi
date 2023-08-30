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
  GuidTest.cpp
  InputTest.cpp)
target_link_libraries(convertTest PRIVATE testShared)
gtest_discover_tests(convertTest)

if(VCPKG_TARGET_TRIPLET AND NOT VCPKG_TARGET_TRIPLET MATCHES [[^.+-static$]])
  add_custom_command(OUTPUT copied_vcpkg_dlls
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
      $<TARGET_FILE:cppgraphqlgen::graphqlservice>
      $<TARGET_FILE:cppgraphqlgen::graphqljson>
      $<TARGET_FILE:cppgraphqlgen::graphqlpeg>
      $<TARGET_FILE:cppgraphqlgen::graphqlresponse>
      $<TARGET_FILE:cppgraphqlgen::graphqlclient>
      $<TARGET_FILE:GTest::gtest>
      $<TARGET_FILE:GTest::gmock>
      ${CMAKE_CURRENT_BINARY_DIR}
    COMMAND ${CMAKE_COMMAND} -E touch ${CMAKE_CURRENT_BINARY_DIR}/copied_vcpkg_dlls)

  add_custom_target(copy_vcpkg_dlls DEPENDS copied_vcpkg_dlls)

  add_dependencies(schemaTest copy_vcpkg_dlls)
  add_dependencies(convertTest copy_vcpkg_dlls)
endif()

if(BUILD_SHARED_LIBS)
  add_custom_command(OUTPUT copied_gqlmapi_dll
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
      $<TARGET_FILE:gqlmapi>
      ${CMAKE_CURRENT_BINARY_DIR}
    COMMAND ${CMAKE_COMMAND} -E touch ${CMAKE_CURRENT_BINARY_DIR}/copied_gqlmapi_dll
    DEPENDS gqlmapi)

  add_custom_target(copy_gqlmapi_dll DEPENDS copied_gqlmapi_dll)

  add_dependencies(schemaTest copy_gqlmapi_dll)
  add_dependencies(convertTest copy_gqlmapi_dll)
endif()
