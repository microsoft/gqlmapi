# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

# Fetch the https://github.com/microsoft/MAPIStubLibrary repository.
include(FetchContent)
FetchContent_Declare(MAPIStubLibrary
  GIT_REPOSITORY "https://github.com/microsoft/MAPIStubLibrary"
  GIT_TAG "a888542d2e2630bf0da3b3855d198773b61c700e"
  GIT_PROGRESS TRUE)
FetchContent_GetProperties(MAPIStubLibrary)
if(NOT mapistublibrary_POPULATED)
  FetchContent_Populate(MAPIStubLibrary)
  file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/cmake DESTINATION ${mapistublibrary_SOURCE_DIR})
  add_subdirectory(${mapistublibrary_SOURCE_DIR}/cmake ${mapistublibrary_BINARY_DIR})
endif()
