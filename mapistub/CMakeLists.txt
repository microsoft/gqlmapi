# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

# Fetch the https://github.com/stephenegriffin/MAPIStubLibrary repository.
include(FetchContent)
FetchContent_Declare(MAPIStubLibrary
  GIT_REPOSITORY "https://github.com/stephenegriffin/MAPIStubLibrary"
  GIT_TAG "01f9bf8cd2ac39078aabfc4662c873159efd75d6"
  GIT_PROGRESS ON)
FetchContent_GetProperties(MAPIStubLibrary)
if(NOT mapistublibrary_POPULATED)
  FetchContent_Populate(MAPIStubLibrary)
  file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/cmake DESTINATION ${mapistublibrary_SOURCE_DIR})
  add_subdirectory(${mapistublibrary_SOURCE_DIR}/cmake ${mapistublibrary_BINARY_DIR})
endif()
