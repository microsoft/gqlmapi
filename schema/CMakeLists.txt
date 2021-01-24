set_property(
  DIRECTORY
  APPEND
  PROPERTY CMAKE_CONFIGURE_DEPENDS schema_files)

add_custom_command(
  OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/schema_files
  COMMAND
    ${CMAKE_COMMAND} "-DSCHEMA_SOURCE_DIR=${CMAKE_CURRENT_SOURCE_DIR}"
    "-DSCHEMA_BINARY_DIR=${CMAKE_CURRENT_BINARY_DIR}"
    "-DSCHEMAGEN_PROGRAM=$<TARGET_FILE:cppgraphqlgen::schemagen>"
    "-DSCHEMA_GRAPHQL=mapi.graphql" "-DSCHEMA_PREFIX=MAPI"
    "-DSCHEMA_NAMESPACE=mapi" -P
    "${CMAKE_CURRENT_SOURCE_DIR}/update_schema.cmake"
  DEPENDS mapi.graphql update_schema.cmake
  COMMENT "Generating GraphQL service")
add_custom_target(update_schema ALL
                  DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/schema_files)

file(STRINGS schema_files SCHEMA_FILES)
add_library(mapischema STATIC ${SCHEMA_FILES})
add_dependencies(mapischema update_schema)
target_include_directories(mapischema PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(
  mapischema
  PUBLIC cppgraphqlgen::graphqljson cppgraphqlgen::graphqlpeg
         cppgraphqlgen::graphqlresponse cppgraphqlgen::graphqlservice)
