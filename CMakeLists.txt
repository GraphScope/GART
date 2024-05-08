cmake_minimum_required(VERSION 3.2...3.5)
project(gart)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

### Forbid in-source builds ###
if("${CMAKE_SOURCE_DIR}" STREQUAL "${CMAKE_BINARY_DIR}")
    message(FATAL_ERROR "In-source builds are not allowed.")
endif("${CMAKE_SOURCE_DIR}" STREQUAL "${CMAKE_BINARY_DIR}")

### Set up compiler flags ###
option(ADD_CONVERTER "Option to add converter" ON)
option(ADD_VEGITO "Option to add Vegito" ON)
option(ADD_PGQL "Option to add PGQL" ON)
option(ADD_GAE_ENGINE "Option to add GAE engine" OFF)

if (ADD_CONVERTER)
    add_subdirectory(converter)
endif()

if (ADD_VEGITO)
    add_subdirectory(vegito)
endif()

if (ADD_PGQL)
    add_subdirectory(pgql)
endif()

if (ADD_GAE_ENGINE)
    add_subdirectory(apps/analytical_engine apps)
endif()

### Copy test schema ###
file(COPY ${CMAKE_SOURCE_DIR}/vegito/test/schema DESTINATION ${CMAKE_BINARY_DIR})

### Create symbolic links for scripts ###
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/scripts)
file(CREATE_LINK ${CMAKE_SOURCE_DIR}/scripts/gart gart SYMBOLIC)
file(CREATE_LINK ${CMAKE_SOURCE_DIR}/scripts/stop-gart stop-gart SYMBOLIC)
file(CREATE_LINK ${CMAKE_SOURCE_DIR}/scripts/check_process.sh check_process.sh SYMBOLIC)
file(CREATE_LINK ${CMAKE_SOURCE_DIR}/scripts/extract_table_schema.py scripts/extract_table_schema.py SYMBOLIC)
file(CREATE_LINK ${CMAKE_SOURCE_DIR}/scripts/distributed_deployment.py scripts/distributed_deployment.py SYMBOLIC)
file(CREATE_LINK ${CMAKE_SOURCE_DIR}/scripts/launch_etcd.py scripts/launch_etcd.py SYMBOLIC)
file(CREATE_LINK ${CMAKE_SOURCE_DIR}/scripts/update_kafka_config_file.py scripts/update_kafka_config_file.py SYMBOLIC)
### install headers ###

set(CMAKE_INSTALL_INCLUDE_DIR "/usr/local/include")

file(GLOB FRAGMENT_HEADER_FILES
          "vegito/include/fragment/id_parser.h"
          )

add_library(frag_header INTERFACE)

set_target_properties(frag_header PROPERTIES
    PUBLIC_HEADER "${FRAGMENT_HEADER_FILES}"
)

install(TARGETS frag_header
    # RUNTIME DESTINATION /usr/local/bin
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDE_DIR}/gart/fragment
)

file(GLOB SEGGRAPH_HEADER_FILES
          "vegito/include/seggraph/blocks.hpp"
          "vegito/include/seggraph/bloom_filter.hpp"
          "vegito/include/seggraph/epoch_graph_reader.hpp"
          "vegito/include/seggraph/types.hpp"
          "vegito/include/seggraph/utils.hpp"
          )

add_library(seggraph_header INTERFACE)

set_target_properties(seggraph_header PROPERTIES
    PUBLIC_HEADER "${SEGGRAPH_HEADER_FILES}"
)

install(TARGETS seggraph_header
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDE_DIR}/gart/seggraph
)

file(GLOB INTERFACE_HEADER_FILES
          "interfaces/fragment/*.h"
          )

add_library(interface_header INTERFACE)

set_target_properties(interface_header PROPERTIES
    PUBLIC_HEADER "${INTERFACE_HEADER_FILES}"
)

install(TARGETS interface_header
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDE_DIR}/gart/interfaces/fragment
)

file(GLOB UTIL_HEADER_FILES
          "vegito/include/util/inline_str.h"
          "vegito/include/util/macros.h"
          "vegito/include/util/serializer.h"
          "vegito/include/util/varint.h"
          "vegito/include/util/bitset.h"
          "vegito/include/util/status.h"
          )

add_library(util_header INTERFACE)

set_target_properties(util_header PROPERTIES
    PUBLIC_HEADER "${UTIL_HEADER_FILES}"
)

install(TARGETS util_header
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDE_DIR}/gart/util
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

### make docs ###
set(GART_DOC_SOURCE_DIR "${CMAKE_SOURCE_DIR}/docs")
add_custom_target(gart-docs
        COMMAND make html
        WORKING_DIRECTORY ${GART_DOC_SOURCE_DIR}
        COMMENT "Building documentation."
        VERBATIM)

### install PostgreSQL Extension ###
# 设置插件的源码目录
set(PGX_PLUGIN_SOURCE_DIR "${CMAKE_SOURCE_DIR}/apps/pgx")

# 添加自定义目标pgx
add_custom_target(pgx
    COMMAND ${CMAKE_MAKE_PROGRAM} USE_PGXS=1 -j
    WORKING_DIRECTORY ${PGX_PLUGIN_SOURCE_DIR}
)

add_custom_target(pgx-install
    COMMAND sudo ${CMAKE_MAKE_PROGRAM} install
    WORKING_DIRECTORY ${PGX_PLUGIN_SOURCE_DIR}
    DEPENDS pgx # Enusre pgx target is built before installing
)
