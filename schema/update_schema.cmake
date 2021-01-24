# Normalize the filesystem paths
get_filename_component(SCHEMA_SOURCE_DIR ${SCHEMA_SOURCE_DIR} ABSOLUTE)
get_filename_component(SCHEMA_BINARY_DIR ${SCHEMA_BINARY_DIR} ABSOLUTE)
get_filename_component(SCHEMAGEN_PROGRAM ${SCHEMAGEN_PROGRAM} ABSOLUTE)
get_filename_component(SCHEMA_GRAPHQL "${SCHEMA_SOURCE_DIR}/${SCHEMA_GRAPHQL}"
                       ABSOLUTE)

# Cleanup all of the stale files in the binary directory
file(GLOB PREVIOUS_FILES ${SCHEMA_BINARY_DIR}/*.h ${SCHEMA_BINARY_DIR}/*.cpp
     ${SCHEMA_BINARY_DIR}/schema_files)
foreach(PREVIOUS_FILE ${PREVIOUS_FILES})
  file(REMOVE ${PREVIOUS_FILE})
endforeach()

# Regenerate the sources in the binary directory
execute_process(
  COMMAND
    ${SCHEMAGEN_PROGRAM} "--schema=${SCHEMA_GRAPHQL}"
    "--prefix=${SCHEMA_PREFIX}" "--namespace=${SCHEMA_NAMESPACE}"
    --separate-files --no-stubs
  OUTPUT_FILE schema_files
  WORKING_DIRECTORY ${SCHEMA_BINARY_DIR})

# Get the up-to-date list of files in the binary directory
set(FILE_NAMES "")
file(GLOB NEW_FILES ${SCHEMA_BINARY_DIR}/*.h ${SCHEMA_BINARY_DIR}/*.cpp)
foreach(NEW_FILE ${NEW_FILES})
  get_filename_component(NEW_FILE ${NEW_FILE} NAME)
  list(APPEND FILE_NAMES "${NEW_FILE}")
endforeach()

# Don't update the files in the source directory if no files were generated in the binary directory.
if(NOT FILE_NAMES)
  message(FATAL_ERROR "Schema generation failed!")
endif()

# Remove stale files in the source directory
cmake_policy(SET CMP0057 NEW)
file(GLOB OLD_FILES ${SCHEMA_SOURCE_DIR}/*.h ${SCHEMA_SOURCE_DIR}/*.cpp)
foreach(OLD_FILE ${OLD_FILES})
  get_filename_component(OLD_FILE ${OLD_FILE} NAME)
  if(NOT OLD_FILE IN_LIST FILE_NAMES)
    file(REMOVE "${SCHEMA_SOURCE_DIR}/${OLD_FILE}")
  endif()
endforeach()

# Copy new and modified files from the binary directory to the source directory
execute_process(
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
          ${SCHEMA_BINARY_DIR}/schema_files ${NEW_FILES} ${SCHEMA_SOURCE_DIR})
