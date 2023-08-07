cmake_minimum_required(VERSION 3.2)
project(vegito)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option (WITH_TEST "build for test" OFF)

### Forbid in-source builds ###
if("${CMAKE_SOURCE_DIR}" STREQUAL "${CMAKE_BINARY_DIR}")
    message(FATAL_ERROR "In-source builds are not allowed.")
endif("${CMAKE_SOURCE_DIR}" STREQUAL "${CMAKE_BINARY_DIR}")

#### Compile choice ####
set(DEFAULT_BUILD_TYPE "Release")
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    message(WARNING "Setting build type to '${DEFAULT_BUILD_TYPE}' as none was specified.")
    set(CMAKE_BUILD_TYPE "${DEFAULT_BUILD_TYPE}" CACHE
        STRING "Choose the type of build." FORCE
    )
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS
        "Debug" "Release" "MinSizeRel" "RelWithDebInfo"
    )
else()
    message(STATUS "Setting build type to '${CMAKE_BUILD_TYPE}'.")
endif()

include(CheckCXXCompilerFlag)
check_cxx_compiler_flag("-march=native" COMPILER_SUPPORTS_MARCH_NATIVE)
if(COMPILER_SUPPORTS_MARCH_NATIVE)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=native")
else()
    message(WARNING "The compiler ${CMAKE_CXX_COMPILER} has no march native support.")
endif()

# seperate install
set(CMAKE_SKIP_INSTALL_ALL_DEPENDENCY true)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O2 -g3")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -O0 -g2")

#### Third-party dependency ####

# find rdkafka
include("cmake/FindRdkafka.cmake")
if (RDKAFKA_FOUND)
    include_directories(${RDKAFKA_INCLUDE_DIR})
    link_libraries(${RDKAFKA_LIBRARIES})
else ()
    message(SEND_ERROR "rdkafka not found")
endif ()

# find GFlags
include("cmake/FindGFlags.cmake")
if (GFLAGS_FOUND)
    include_directories(${GFLAGS_INCLUDE_DIRS})
    link_libraries(${GFLAGS_LIBRARIES})
else ()
    message(SEND_ERROR "gflags not found")
endif ()

# find TBB
include("cmake/FindTBB.cmake")
if (TBB_FOUND)
    include_directories(${TBB_INCLUDE_DIRS})
    link_directories(${TBB_LIBRARY_DIRS})
    link_libraries(${TBB_LIBRARIES})
else ()
    message(SEND_ERROR "TBB not found")
endif ()

# find etcd-cpp-api
find_package(etcd-cpp-api REQUIRED)
include_directories(${ETCD_CPP_INCLUDE_DIR})
link_libraries(${ETCD_CPP_LIBRARIES})

# find vineyard
find_package(vineyard 0.15.0 REQUIRED)
include_directories(${VINEYARD_INCLUDE_DIRS})
link_libraries(${VINEYARD_LIBRARIES})

# find yaml-cpp
find_package(yaml-cpp REQUIRED)
include_directories(${YAML_CPP_INCLUDE_DIR})
link_libraries(${YAML_CPP_LIBRARIES})

#### Headers ####

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src)

### add main files ###

file(GLOB SOURCES
          "src/system_flags.cc" "src/framework/*.cc"
          "src/graph/*.cc" "src/graph/graph_ops/*.cc"
          "src/property/*.cc" "src/seggraph/*.cpp"
          )

file(GLOB_RECURSE FILES_NEED_FORMAT "*.cc" "*.h" "*.cpp" "*.hpp")

add_custom_target(vegito_clformat
          COMMAND clang-format --style=file -i ${FILES_NEED_FORMAT}
          COMMENT "Running clang-format."
          VERBATIM)

add_executable(vegito "src/main.cc"
               ${SOURCES}
               )

add_executable(load_graph_test "test/load_graph_test.cc"
               ${SOURCES}
               )

target_compile_definitions(load_graph_test PUBLIC -DWITH_TEST)
