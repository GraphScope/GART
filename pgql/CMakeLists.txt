cmake_minimum_required(VERSION 3.2)

set(GART_PGQL_MAJOR_VERSION 0)
set(GART_PGQL_MINOR_VERSION 1)
set(GART_PGQL_VERSION ${GART_PGQL_MAJOR_VERSION}.${GART_PGQL_MINOR_VERSION})

project(gart_pgql VERSION ${GART_PGQL_VERSION})

### Forbid in-source builds ###
if("${CMAKE_SOURCE_DIR}" STREQUAL "${CMAKE_BINARY_DIR}")
    message(FATAL_ERROR "In-source builds are not allowed.")
endif("${CMAKE_SOURCE_DIR}" STREQUAL "${CMAKE_BINARY_DIR}")

file(GLOB_RECURSE JAVA_FILES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "*.java")
file(GLOB_RECURSE POM_FILE "pom.xml")

add_custom_target(pgql
    COMMAND mvn -f ${POM_FILE} clean package
    DEPENDS ${JAVA_FILES} ${POM_FILE}
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    VERBATIM
)
