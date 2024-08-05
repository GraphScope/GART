cmake_minimum_required(VERSION 3.2...3.5)

set(BINLOG_CONVERT_MAJOR_VERSION 0)
set(BINLOG_CONVERT_MINOR_VERSION 1)
set(BINLOG_CONVERT_VERSION ${BINLOG_CONVERT_MAJOR_VERSION}.${BINLOG_CONVERT_MINOR_VERSION})

project(binlog_convert LANGUAGES C CXX VERSION ${BINLOG_CONVERT_VERSION})

# Set flags
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c99")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -g")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -O0 -g2")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3 -g")

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE "Release" CACHE STRING "The default build type is Release" FORCE)
endif()

option (USE_TBB "use TBB for concurrent queue?" OFF)
option (USE_FLAT_HASH_MAP "use absl::flat_hash_map?" ON)

if(USE_TBB)
  add_definitions(-DUSE_TBB)
endif()

if (USE_FLAT_HASH_MAP)
  add_definitions(-DUSE_FLAT_HASH_MAP)
endif()


# ------------------------------------------------------------------------------
# find_libraries
# ------------------------------------------------------------------------------
find_package(vineyard REQUIRED)
include_directories(${VINEYARD_INCLUDE_DIRS})

find_package(yaml-cpp REQUIRED)
include_directories(${YAML_CPP_INCLUDE_DIR})

# find etcd-cpp-api
find_package(etcd-cpp-api REQUIRED)
include_directories(${ETCD_CPP_INCLUDE_DIR})
link_libraries(${ETCD_CPP_LIBRARIES})

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

if(USE_TBB)
  # find TBB
  include("../vegito/cmake/FindTBB.cmake")
  if (TBB_FOUND)
      include_directories(${TBB_INCLUDE_DIRS})
      link_directories(${TBB_LIBRARY_DIRS})
      link_libraries(${TBB_LIBRARIES})
  else ()
      message(SEND_ERROR "TBB not found")
  endif ()
endif ()

if(ENABLE_CHECKPOINT)
  find_package(Boost REQUIRED COMPONENTS)
  include_directories(${Boost_INCLUDE_DIRS})
endif()

if (USE_FLAT_HASH_MAP)
  add_subdirectory(../third_party/abseil-cpp abseil-cpp)
endif()

file(GLOB_RECURSE FILES_NEED_FORMAT "*.cc" "*.h")

add_custom_target(convert_clformat
        COMMAND clang-format --style=file -i ${FILES_NEED_FORMAT}
        COMMENT "Running clang-format."
        VERBATIM)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/..)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../vegito/include)

add_executable(binlog_convert_debezium binlog_convert.cc flags.cc parser.cc)

target_include_directories(binlog_convert_debezium PRIVATE ${RDKAFKA_INCLUDE_DIR})
target_link_libraries(binlog_convert_debezium ${RDKAFKA_LIBRARIES} ${GFLAGS_LIBRARIES} ${GLOG_LIBRARIES} ${CMAKE_DL_LIBS} ${YAML_CPP_LIBRARIES})
target_compile_definitions(binlog_convert_debezium PUBLIC -DUSE_DEBEZIUM)

if(ENABLE_CHECKPOINT)
  target_compile_definitions(binlog_convert_debezium PRIVATE ENABLE_CHECKPOINT)
  target_link_libraries(binlog_convert_debezium ${Boost_LIBRARIES})
endif()

if (USE_FLAT_HASH_MAP)
  target_link_libraries(binlog_convert_debezium absl::flat_hash_map)
endif()

add_executable(binlog_convert_maxwell binlog_convert.cc flags.cc parser.cc)

target_include_directories(binlog_convert_maxwell PRIVATE ${RDKAFKA_INCLUDE_DIR})
target_link_libraries(binlog_convert_maxwell ${RDKAFKA_LIBRARIES} ${GFLAGS_LIBRARIES} ${GLOG_LIBRARIES} ${CMAKE_DL_LIBS} ${YAML_CPP_LIBRARIES})

add_executable(json2yaml json2yaml.cc)
target_link_libraries(json2yaml ${YAML_CPP_LIBRARIES})

if(ENABLE_CHECKPOINT)
  target_compile_definitions(binlog_convert_maxwell PRIVATE ENABLE_CHECKPOINT)
  target_link_libraries(binlog_convert_maxwell ${Boost_LIBRARIES})
endif()

if (USE_FLAT_HASH_MAP)
  target_link_libraries(binlog_convert_maxwell absl::flat_hash_map)
endif()