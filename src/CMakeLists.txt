# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

# Build gqlmapiCommon as a static lib
add_library(gqlmapiCommon STATIC
  MAPIGuid.cpp
  Session.cpp
  Query.cpp
  Mutation.cpp
  Subscription.cpp
  Store.cpp
  Folder.cpp
  Item.cpp
  Property.cpp
  IntId.cpp
  NamedId.cpp
  StringId.cpp
  IntValue.cpp
  BoolValue.cpp
  StringValue.cpp
  GuidValue.cpp
  DateTimeValue.cpp
  BinaryValue.cpp
  CheckResult.cpp
  Unicode.cpp
  Guid.cpp
  DateTime.cpp
  TableDirectives.cpp
  ItemAdded.cpp
  ItemUpdated.cpp
  ItemRemoved.cpp
  ItemsReloaded.cpp
  ItemsSubscription.cpp
  FolderAdded.cpp
  FolderUpdated.cpp
  FolderRemoved.cpp
  FoldersReloaded.cpp
  SubFoldersSubscription.cpp
  RootFoldersSubscription.cpp)
target_include_directories(gqlmapiCommon PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>)
target_link_libraries(gqlmapiCommon PUBLIC
  mapistub
  mapischema
  cppgraphqlgen::graphqlintrospection
  cppgraphqlgen::graphqljson)

# Build a separate gqlmapi library as either static or shared depending on BUILD_SHARED_LIBS.
add_library(gqlmapi Service.cpp)
target_link_libraries(gqlmapi
  PRIVATE gqlmapiCommon
  INTERFACE
    cppgraphqlgen::graphqlintrospection
    cppgraphqlgen::graphqljson)
target_include_directories(gqlmapi PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../schema>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>)

if(BUILD_SHARED_LIBS)
  target_compile_definitions(gqlmapi PUBLIC GQLMAPI_DLLEXPORTS)
else()
  install(TARGETS mapistub gqlmapiCommon EXPORT gqlmapi)
endif()

file(GLOB PUBLIC_HEADERS RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/include/*.h)
set_property(TARGET gqlmapi PROPERTY PUBLIC_HEADER ${PUBLIC_HEADERS})

install(TARGETS gqlmapi EXPORT gqlmapi)
install(EXPORT gqlmapi DESTINATION cmake)