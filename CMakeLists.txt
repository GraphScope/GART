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
add_subdirectory(apps/analytical_engine apps)

file(COPY ${CMAKE_SOURCE_DIR}/scripts/gart DESTINATION ${CMAKE_BINARY_DIR})
file(COPY ${CMAKE_SOURCE_DIR}/scripts/stop-gart DESTINATION ${CMAKE_BINARY_DIR})
file(COPY ${CMAKE_SOURCE_DIR}/scripts/extract_table_schema.py DESTINATION ${CMAKE_BINARY_DIR}/scripts)
file(COPY ${CMAKE_SOURCE_DIR}/vegito/test/schema DESTINATION ${CMAKE_BINARY_DIR})