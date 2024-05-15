cmake_minimum_required(VERSION 3.4...3.18)
project(local_gart)

# Set flags
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c99")

set(DEFAULT_BUILD_TYPE "Release")
if (NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    message(STATUS "Setting build type to '${DEFAULT_BUILD_TYPE}' as none was specified.")
    set(CMAKE_BUILD_TYPE "${DEFAULT_BUILD_TYPE}" CACHE STRING "Choose the type of build." FORCE)
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
endif ()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -g")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -O0 -g2")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3 -g")

include(CheckCXXCompilerFlag)
check_cxx_compiler_flag("-march=native" COMPILER_SUPPORTS_MARCH_NATIVE)
if(COMPILER_SUPPORTS_MARCH_NATIVE)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=native")
else()
    message(STATUS "The compiler ${CMAKE_CXX_COMPILER} has no march native support.")
endif()

find_package(MPI REQUIRED)
include_directories(SYSTEM ${MPI_CXX_INCLUDE_PATH})

find_package(OpenMP REQUIRED)

find_package(libgrapelite REQUIRED)
include_directories(${LIBGRAPELITE_INCLUDE_DIRS})

find_package(vineyard 0.15.0 REQUIRED)
include_directories(${VINEYARD_INCLUDE_DIRS})
link_libraries(${VINEYARD_LIBRARIES})


find_package(etcd-cpp-api REQUIRED)
include_directories(${ETCD_CPP_INCLUDE_DIR})
link_libraries(${ETCD_CPP_LIBRARIES})

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../../vegito/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../..)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../networkx)
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

# Find pybind11
find_package(pybind11 REQUIRED)

pybind11_add_module(local_gart SHARED python_bindings.cc fragment_builder.cc)

# Attempt to find python3, fallback to python if not found
find_program(PYTHON_EXECUTABLE NAMES python3 python)

if(NOT PYTHON_EXECUTABLE)
    message(FATAL_ERROR "Python is not found")
endif()

# Reflect our choice of Python executable in the command
message(STATUS "Using Python executable: ${PYTHON_EXECUTABLE}")

execute_process(COMMAND ${PYTHON_EXECUTABLE} -c "import site; print(site.getsitepackages()[0])"
  OUTPUT_VARIABLE PYTHON_SITE_PACKAGES OUTPUT_STRIP_TRAILING_WHITESPACE)

# Install the module
install(TARGETS local_gart LIBRARY DESTINATION ${PYTHON_SITE_PACKAGES})
