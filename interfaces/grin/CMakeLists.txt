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
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../vegito/src)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

file(GLOB_RECURSE FILES_NEED_FORMAT "src/*.cc")

add_custom_target(grin_clformat
        COMMAND clang-format --style=file -i ${FILES_NEED_FORMAT}
        COMMENT "Running clang-format."
        VERBATIM)

file(GLOB SOURCES "src/*.cc" "src/topology/*.cc" "src/partition/*.cc" "src/property/*.cc" "src/index/*.cc" "src/common/*.cc" "include/extension/src/*.cc")
add_library(gart_grin SHARED ${SOURCES})
target_link_libraries(gart_grin ${LIBGRAPELITE_LIBRARIES} ${GFLAGS_LIBRARIES} ${CMAKE_DL_LIBS} ${VINEYARD_LIBRARIES} ${MPI_CXX_LIBRARIES})


add_executable(run_grin_test test/test.cc)

target_include_directories(run_grin_test PRIVATE ${LIBGRAPELITE_INCLUDE_DIRS}/grape/analytical_apps fragment)
target_link_libraries(run_grin_test gart_grin ${LIBGRAPELITE_LIBRARIES} ${GFLAGS_LIBRARIES} ${CMAKE_DL_LIBS} ${MPI_CXX_LIBRARIES})
target_link_libraries(run_grin_test OpenMP::OpenMP_CXX)
