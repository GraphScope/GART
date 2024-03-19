cmake_minimum_required(VERSION 3.4...3.18)
project(local_gart)

# Set flags
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c99")

set(DEFAULT_BUILD_TYPE "Release")
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
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

# Find pybind11
find_package(pybind11 REQUIRED)

# Add your module (the name here should match the module name given in PYBIND11_MODULE)
pybind11_add_module(local_gart SHARED python_bindings.cc fragment_builder.cc)

execute_process(COMMAND python -c "import site; print(site.getsitepackages()[0])"
  OUTPUT_VARIABLE PYTHON_SITE_PACKAGES OUTPUT_STRIP_TRAILING_WHITESPACE)

# Install the module
install(TARGETS local_gart LIBRARY DESTINATION ${PYTHON_SITE_PACKAGES})
