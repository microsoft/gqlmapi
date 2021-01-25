# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

# Build gqlmapi as a static lib too
add_library(gqlmapi STATIC
  Service.cpp
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
target_include_directories(gqlmapi
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(gqlmapi
  PRIVATE mapistub
  PUBLIC mapischema cppgraphqlgen::graphqlintrospection cppgraphqlgen::graphqljson)