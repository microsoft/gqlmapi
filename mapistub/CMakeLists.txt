# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

# Fetch the https://github.com/stephenegriffin/MAPIStubLibrary repository.
include(FetchContent)
FetchContent_Declare(MAPIStubLibrary
  GIT_REPOSITORY "https://github.com/wravery/MAPIStubLibrary"
  GIT_TAG "87dbfba2d44302736933e9e6cd8787995568de27"
  GIT_PROGRESS TRUE)
FetchContent_GetProperties(MAPIStubLibrary)
if(NOT mapistublibrary_POPULATED)
  FetchContent_Populate(MAPIStubLibrary)
  file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/cmake DESTINATION ${mapistublibrary_SOURCE_DIR})
  add_subdirectory(${mapistublibrary_SOURCE_DIR}/cmake ${mapistublibrary_BINARY_DIR})
endif()
