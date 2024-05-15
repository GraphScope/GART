cmake_minimum_required(VERSION 3.1)

set(GART_NETWORKX_MAJOR_VERSION 0)
set(GART_NETWORKX_MINOR_VERSION 1)
set(GART_NETWORKX_VERSION ${GART_NETWORKX_MAJOR_VERSION}.${GART_NETWORKX_MINOR_VERSION})

project(gart_networkx LANGUAGES C CXX VERSION ${GART_NETWORKX_VERSION})

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


find_package(Protobuf REQUIRED)
include_directories(${Protobuf_INCLUDE_DIRS})
include_directories(${CMAKE_CURRENT_BINARY_DIR})

find_package(gRPC QUIET)
if(gRPC_FOUND)
    set(GRPC_LIBRARIES gRPC::gpr gRPC::grpc gRPC::grpc++)
    get_target_property(GRPC_CPP_PLUGIN gRPC::grpc_cpp_plugin LOCATION)
    get_target_property(GRPC_INCLUDE_DIR gRPC::grpc INTERFACE_INCLUDE_DIRECTORIES)
else()
    include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/FindGRPC.cmake)
    set(GRPC_LIBRARIES ${GPR_LIBRARY} ${GRPC_LIBRARY} ${GRPC_GRPC++_LIBRARY})
endif()

# Attempt to find python3, fallback to python if not found
find_program(PYTHON_EXECUTABLE NAMES python3 python)

if(NOT PYTHON_EXECUTABLE)
    message(FATAL_ERROR "Python is not found")
endif()

# Reflect our choice of Python executable in the command
message(STATUS "Using Python executable: ${PYTHON_EXECUTABLE}")

# Attempt to find pip3 associated with the found python, fallback to pip if not found
find_program(PIP_EXECUTABLE NAMES pip3 pip HINTS "${PYTHON_EXECUTABLE}")
if(NOT PIP_EXECUTABLE)
    message(FATAL_ERROR "pip is not found")
endif()

# Reflect our choice of pip executable in the command
message(STATUS "Using pip executable: ${PIP_EXECUTABLE}")

# Generate proto for cpp
execute_process(COMMAND ${PYTHON_EXECUTABLE} ./proto/proto_generator.py "${CMAKE_CURRENT_SOURCE_DIR}/build" --cpp WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

file(GLOB PROTO_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/build/*.cc)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/server)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../interfaces)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../vegito/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../..)

find_package(MPI REQUIRED)
include_directories(SYSTEM ${MPI_CXX_INCLUDE_PATH})

find_package(OpenMP REQUIRED)

find_package(libgrapelite REQUIRED)
include_directories(${LIBGRAPELITE_INCLUDE_DIRS})

find_package(RapidJSON REQUIRED)
if (DEFINED RapidJSON_INCLUDE_DIRS)  # rapidjson > 1.1.0
    include_directories(${RapidJSON_INCLUDE_DIRS})
else ()  # rapidjson <= 1.1.0
    include_directories(${RAPIDJSON_INCLUDE_DIRS})
endif ()
add_definitions(-DRAPIDJSON_HAS_CXX11=1)
add_definitions(-DRAPIDJSON_HAS_STDSTRING=1)
add_definitions(-DRAPIDJSON_HAS_CXX11_RVALUE_REFS=1)
add_definitions(-DRAPIDJSON_HAS_CXX11_RANGE_FOR=1)

find_package(msgpack REQUIRED)

find_package(vineyard 0.15.0 REQUIRED)
include_directories(${VINEYARD_INCLUDE_DIRS})
link_libraries(${VINEYARD_LIBRARIES})

# find etcd-cpp-api
find_package(etcd-cpp-api REQUIRED)
include_directories(${ETCD_CPP_INCLUDE_DIR})
link_libraries(${ETCD_CPP_LIBRARIES})

# find Boost
find_package(Boost REQUIRED COMPONENTS system filesystem
             # required by folly
             context program_options regex thread)
include_directories(SYSTEM ${Boost_INCLUDE_DIRS})
link_libraries(${Boost_LIBRARIES})

# find GFlags
include("../../vegito/cmake/FindGFlags.cmake")
if (GFLAGS_FOUND)
    include_directories(${GFLAGS_INCLUDE_DIRS})
    link_libraries(${GFLAGS_LIBRARIES})
else ()
    message(SEND_ERROR "gflags not found")
endif ()

file(GLOB SOURCES
          "server/utils/*.cc"
          "server/graph_server_flags.cc"
          )

set_source_files_properties(${PROTO_SOURCES} PROPERTIES COMPILE_FLAGS -Wno-array-bounds)
add_executable(gart_networkx_server server/graph_server.cc ${PROTO_SOURCES} ${SOURCES})
target_link_libraries(gart_networkx_server ${GRPC_LIBRARIES} ${Protobuf_LIBRARIES} ${MPI_CXX_LIBRARIES} ${LIBGRAPELITE_LIBRARIES})

add_custom_target(create_pybind_build_dir
    COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_CURRENT_SOURCE_DIR}/python_bindings/build"
    )

add_custom_target(install_pybind
    COMMAND cmake .. WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/python_bindings/build DEPENDS create_pybind_build_dir
    COMMAND make -j WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/python_bindings/build
    COMMAND make install WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/python_bindings/build
    )


add_custom_target(install_python
        COMMAND ${PYTHON_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/client/python/setup.py build_proto sdist bdist_wheel WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/client/python
        COMMAND ${PIP_EXECUTABLE} install . --force-reinstall WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/client/python
        COMMENT "Install python client"
        VERBATIM
        )


add_custom_target(create_build_dir
        COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_CURRENT_SOURCE_DIR}/../analytical_engine/build"
    )

add_custom_target(install_gae
        COMMAND cmake .. WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/../analytical_engine/build DEPENDS create_build_dir
        COMMAND make run_gart_app -j WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/../analytical_engine/build
        COMMAND mkdir -p ${CMAKE_CURRENT_SOURCE_DIR}/build/apps/
        COMMAND cp run_gart_app ${CMAKE_CURRENT_SOURCE_DIR}/build/apps/ WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/../analytical_engine/build
        COMMENT "Install GAE app"
        VERBATIM)
