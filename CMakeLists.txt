cmake_minimum_required(VERSION 3.2)
project(gart)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

### Forbid in-source builds ###
if("${CMAKE_SOURCE_DIR}" STREQUAL "${CMAKE_BINARY_DIR}")
    message(FATAL_ERROR "In-source builds are not allowed.")
endif("${CMAKE_SOURCE_DIR}" STREQUAL "${CMAKE_BINARY_DIR}")

add_subdirectory(converter)
add_subdirectory(vegito)
# add_subdirectory(apps/analytical_engine apps)

### Copy test schema ###
file(COPY ${CMAKE_SOURCE_DIR}/vegito/test/schema DESTINATION ${CMAKE_BINARY_DIR})

### Create symbolic links for scripts ###
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/scripts)
file(CREATE_LINK ${CMAKE_SOURCE_DIR}/scripts/gart gart SYMBOLIC)
file(CREATE_LINK ${CMAKE_SOURCE_DIR}/scripts/stop-gart stop-gart SYMBOLIC)
file(CREATE_LINK ${CMAKE_SOURCE_DIR}/scripts/extract_table_schema.py scripts/extract_table_schema.py SYMBOLIC)

### install headers ###

set(CMAKE_INSTALL_INCLUDEDIR "/usr/local/include")

file(GLOB FRAGMENT_HEADER_FILES
          "vegito/include/fragment/id_parser.h"
          )

add_library(frag_header INTERFACE)

set_target_properties(frag_header PROPERTIES
    PUBLIC_HEADER "${FRAGMENT_HEADER_FILES}"
)

install(TARGETS frag_header
    # RUNTIME DESTINATION /usr/local/bin
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/gart/fragment
)

file(GLOB SEGGRAPH_HEADER_FILES
          "vegito/include/seggraph/*.hpp"
          )

add_library(seggraph_header INTERFACE)

set_target_properties(seggraph_header PROPERTIES
    PUBLIC_HEADER "${SEGGRAPH_HEADER_FILES}"
)

install(TARGETS seggraph_header
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/gart/seggraph
)

file(GLOB INTERFACE_HEADER_FILES
          "interfaces/fragment/*.h"
          )

add_library(interface_header INTERFACE)

set_target_properties(interface_header PROPERTIES
    PUBLIC_HEADER "${INTERFACE_HEADER_FILES}"
)

install(TARGETS interface_header
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/gart/interfaces/fragment
)

file(GLOB UTIL_HEADER_FILES
          "vegito/include/util/*.h"
          )

add_library(util_header INTERFACE)

set_target_properties(util_header PROPERTIES
    PUBLIC_HEADER "${UTIL_HEADER_FILES}"
)

install(TARGETS util_header
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/gart/util
)

### check format ###
file(GLOB_RECURSE FILES_NEED_FORMAT "*.cc" "*.h" "*.cpp" "*.hpp")

add_custom_target(clformat
        COMMAND clang-format --style=file -i ${FILES_NEED_FORMAT}
        COMMENT "Running clang-format."
        VERBATIM)

add_custom_target(pyformat
        COMMAND python3 -m black ${CMAKE_SOURCE_DIR}
        COMMENT "Running black for python formats."
        VERBATIM)
