cmake_minimum_required(VERSION 3.1)

set(BINLOG_CONVERT_MAJOR_VERSION 0)
set(BINLOG_CONVERT_MINOR_VERSION 1)
set(BINLOG_CONVERT_VERSION ${BINLOG_CONVERT_MAJOR_VERSION}.${BINLOG_CONVERT_MINOR_VERSION})

project(binlog_convert LANGUAGES C CXX VERSION ${BINLOG_CONVERT_VERSION})

# Set flags
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c99")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O2 -g3")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -O0 -g2")

# ------------------------------------------------------------------------------
# find_libraries
# ------------------------------------------------------------------------------
find_package(vineyard 0.15.0 REQUIRED)
include_directories(${VINEYARD_INCLUDE_DIRS})

find_package(yaml-cpp REQUIRED)
include_directories(${YAML_CPP_INCLUDE_DIR})

include("../vegito/cmake/FindRdkafka.cmake")
if (NOT RDKAFKA_FOUND)
  message(SEND_ERROR "rdkafka not found")
endif ()

include("../vegito/cmake/FindGFlags.cmake")
if (GFLAGS_FOUND)
    include_directories(SYSTEM ${GFLAGS_INCLUDE_DIRS})
else ()
    message(SEND_ERROR "gflags not found")
endif ()

include("../vegito/cmake/FindGlog.cmake")
include_directories(SYSTEM ${GLOG_INCLUDE_DIRS})
if (GLOG_FOUND)
    set(CMAKE_REQUIRED_INCLUDES "${GLOG_INCLUDE_DIRS}")
    set(CMAKE_REQUIRED_LIBRARIES "${CMAKE_REQUIRED_LIBRARIES} ${GLOG_LIBRARIES}")
endif ()

file(GLOB_RECURSE FILES_NEED_FORMAT "*.cc" "*.h")

add_custom_target(convert_clformat
        COMMAND clang-format --style=file -i ${FILES_NEED_FORMAT}
        COMMENT "Running clang-format."
        VERBATIM)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/..)

add_executable(binlog_convert binlog_convert.cc flags.cc parser.cc)

target_include_directories(binlog_convert PRIVATE ${RDKAFKA_INCLUDE_DIR})
target_link_libraries(binlog_convert ${RDKAFKA_LIBRARIES} ${GFLAGS_LIBRARIES} ${GLOG_LIBRARIES} ${CMAKE_DL_LIBS} ${YAML_CPP_LIBRARIES})
# target_compile_definitions(binlog_convert PUBLIC -DUSE_DEBEZIUM)

add_executable(json2yaml json2yaml.cc)
target_link_libraries(json2yaml ${YAML_CPP_LIBRARIES})