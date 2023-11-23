cmake_minimum_required(VERSION 3.1)

set(GRIN_READER_MAJOR_VERSION 0)
set(GRIN_READER_MINOR_VERSION 1)
set(GRIN_READER_VERSION ${GRIN_READER_MAJOR_VERSION}.${GRIN_READER_MINOR_VERSION})

project(grin_reader LANGUAGES C CXX VERSION ${GRIN_READER_VERSION})

# Set flags
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c99")

set(DEFAULT_BUILD_TYPE "Release")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -g")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -O0 -g")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3 -g")

# ------------------------------------------------------------------------------
# find_libraries
# ------------------------------------------------------------------------------

find_package(MPI REQUIRED)
include_directories(SYSTEM ${MPI_CXX_INCLUDE_PATH})

find_package(OpenMP REQUIRED)

find_package(libgrapelite REQUIRED)
include_directories(${LIBGRAPELITE_INCLUDE_DIRS})

find_package(vineyard 0.15.0 REQUIRED)
include_directories(${VINEYARD_INCLUDE_DIRS})
link_libraries(${VINEYARD_LIBRARIES})

find_package(Protobuf REQUIRED)
include_directories(${Protobuf_INCLUDE_DIRS})
include_directories(${CMAKE_CURRENT_BINARY_DIR})

PROTOBUF_GENERATE_CPP(PROTO_SRCS PROTO_HDRS include/proto/storage.proto)

include("../../vegito/cmake/FindGFlags.cmake")
if (GFLAGS_FOUND)
    include_directories(SYSTEM ${GFLAGS_INCLUDE_DIRS})
else ()
    message(FATAL_ERROR "gflags not found")
endif ()

include("../../vegito/cmake/FindGlog.cmake")
include_directories(SYSTEM ${GLOG_INCLUDE_DIRS})
if (GLOG_FOUND)
    set(CMAKE_REQUIRED_INCLUDES "${GLOG_INCLUDE_DIRS}")
    set(CMAKE_REQUIRED_LIBRARIES "${CMAKE_REQUIRED_LIBRARIES} ${GLOG_LIBRARIES}")
endif ()

find_package(etcd-cpp-api REQUIRED)
include_directories(${ETCD_CPP_INCLUDE_DIR})
link_libraries(${ETCD_CPP_LIBRARIES})

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/..)
include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../..)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../vegito/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include/extension/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include/include)


file(GLOB SOURCES "src/*.cc" "src/topology/*.cc" "src/partition/*.cc" "src/property/*.cc" "src/index/*.cc" "src/common/*.cc" "include/extension/src/*.cc")
add_library(gart_grin SHARED ${SOURCES} ${PROTO_SRCS} ${PROTO_HDRS})
target_link_libraries(gart_grin ${LIBGRAPELITE_LIBRARIES} ${GFLAGS_LIBRARIES} ${CMAKE_DL_LIBS} ${VINEYARD_LIBRARIES} ${MPI_CXX_LIBRARIES} ${PROTOBUF_LIBRARIES})


add_executable(run_grin_test test/test.cc)

target_include_directories(run_grin_test PRIVATE ${LIBGRAPELITE_INCLUDE_DIRS}/grape/analytical_apps fragment)
target_link_libraries(run_grin_test gart_grin ${LIBGRAPELITE_LIBRARIES} ${GFLAGS_LIBRARIES} ${CMAKE_DL_LIBS} ${MPI_CXX_LIBRARIES})
target_link_libraries(run_grin_test OpenMP::OpenMP_CXX)

file(GLOB_RECURSE FILES_NEED_FORMAT "src/*.cc")

add_custom_target(grin_clformat
        COMMAND clang-format --style=file -i ${FILES_NEED_FORMAT}
        COMMENT "Running clang-format."
        VERBATIM)
