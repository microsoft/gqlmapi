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
  Input.cpp
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
target_include_directories(gqlmapiCommon PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../schema>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>)
target_link_libraries(gqlmapiCommon PUBLIC
  mapistub
  mapi_schema)

# Build a separate gqlmapi library as either static or shared depending on BUILD_SHARED_LIBS.
add_library(gqlmapi Service.cpp)
target_link_libraries(gqlmapi
  PRIVATE $<BUILD_INTERFACE:gqlmapiCommon>
  INTERFACE cppgraphqlgen::graphqlservice)

option(IMPLICIT_GRAPHQLJSON_DEPENDENCY "Make the gqlmapi library depend on cppgraphqlgen::graphqljson implicitly." ON)

if(IMPLICIT_GRAPHQLJSON_DEPENDENCY)
  target_link_libraries(gqlmapi INTERFACE cppgraphqlgen::graphqljson)
endif()

target_include_directories(gqlmapi PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../schema>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>)

if(BUILD_SHARED_LIBS)
  target_compile_definitions(gqlmapi PUBLIC GQLMAPI_DLLEXPORTS)

  install(FILES $<TARGET_FILE_DIR:gqlmapi>/gqlmapi.pdb
    TYPE BIN
    CONFIGURATIONS
      Debug
      RelWithDebInfo)

  install(FILES
    $<TARGET_FILE:cppgraphqlgen::graphqlpeg>
    $<TARGET_FILE:cppgraphqlgen::graphqlresponse>
    $<TARGET_FILE:cppgraphqlgen::graphqlservice>
    TYPE BIN)

  if(IMPLICIT_GRAPHQLJSON_DEPENDENCY)
    install(FILES $<TARGET_FILE:cppgraphqlgen::graphqljson> TYPE BIN)
  endif()
else()
  install(TARGETS
      mapistub
      mapi_schema
      gqlmapiCommon
    EXPORT gqlmapi)
endif()

file(GLOB PUBLIC_HEADERS RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/include/*.h)
set_target_properties(gqlmapi PROPERTIES PUBLIC_HEADER "${PUBLIC_HEADERS}")

install(TARGETS gqlmapi EXPORT gqlmapi)
install(EXPORT gqlmapi DESTINATION cmake)