# cmake configurations for this project, mainly targeting *nix environment
#
# It should 'work', but may or may not follow industry best-practices for
# If in some way it does not, please do help improve it :)

cmake_minimum_required(VERSION 3.12)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if (NOT MSVC)
    set(CMAKE_TOOLCHAIN_FILE "${CMAKE_SOURCE_DIR}/vcpkg/scripts/buildsystems/vcpkg.cmake")
    message("Finished VCPKG integration")
else()
    message(
        FATAL_ERROR
        "Currently not support MSVC with CMake"
        "use Vcpkg and Visual Studio instead")
endif()

project(TxService)
### General configurations

set(CMAKE_CXX_STANDARD 17)

set(CMAKE_CXX_FLAGS "-fconcepts ${CMAKE_CXX_FLAGS}")
set(CMAKE_CXX_FLAGS_DEBUG "-g -fpermissive ${CMAKE_CXX_FLAGS_DEBUG}")
set(CMAKE_CXX_FLAGS_RELEASE "-fpermissive -O2")

SET(CMAKE_BUILD_TYPE "Release")

# Mongo-cxx-driver is compiled with this macro turned off,
# so we have to turn it off in our build to avoid linker error of using a
# different version of STL.
#add_compile_definitions(_GLIBCXX_USE_CXX11_ABI=1)

#add_subdirectory(dep/fugue_redis)

#set(CASS_BUILD_STATIC off)
#add_subdirectory(dep/cpp-driver)

#find_package(RapidJSON CONFIG REQUIRED)
### TxService

set(TXSERVICE_SRC_DIR ${CMAKE_SOURCE_DIR}/src)
set(TXSERVICE_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/include)
set(REDISLIB_INCLUDE_DIR ${CMAKE_BINARY_DIR}/include)
#set(TXSERVICE_EXAMPLE_DIR ${CMAKE_SOURCE_DIR}/examples)

file(GLOB_RECURSE TXSERVICE_SRC ${TXSERVICE_SRC_DIR}/*.cpp ${TXSERVICE_SRC_DIR}/*.cc)
# remove sources under 'tool' directory, which potentially contain other `main`.
list(FILTER TXSERVICE_SRC EXCLUDE REGEX .*/tool/.*\.cpp|.*/checkpoint/.*\.cpp|src/test/tpcc/tpcc-.*-test.cpp|src/redis/redis-load-cluster.cpp|src/test/kickout*|src/test/tpcc/tpcc-memorykv.cpp|src/test/tpcc/reality-kickout-test.cpp|src/cassandra/*)

#find_package(JNI)

add_library(TxService OBJECT ${TXSERVICE_SRC})
#target_link_libraries(TxService PUBLIC Folly::folly fmt::fmt FugueRedis cassandra tcmalloc profiler)

target_link_libraries(TxService PUBLIC Folly::folly)

set_property(TARGET TxService PROPERTY POSITION_INDEPENDENT_CODE ON)
target_include_directories(TxService PUBLIC ${TXSERVICE_INCLUDE_DIR} ${RAPIDJSON_INCLUDE_DIRS})
set_target_properties(
	TxService PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

find_package(folly CONFIG REQUIRED)
find_package(fmt CONFIG REQUIRED) 
 


add_library(TxServiceDyn SHARED /dev/null)
set_property(TARGET TxServiceDyn PROPERTY POSITION_INDEPENDENT_CODE ON)
target_link_libraries(TxServiceDyn PUBLIC TxService)
