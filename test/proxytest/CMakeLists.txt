#lfrb test
cmake_minimum_required(VERSION 3.20)

project(proxy_test)

if (CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O3 -std=gnu2x -ggdb")
    set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS} -ggdb")
    set(CMAKE_CXX_FLAGS "-O3 -std=c++23 -fconcepts -ggdb")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS} -ggdb")
else ()
    message(FATAL_ERROR "unsupported system type")
endif ()


message(STATUS "CMAKE_SOURCE_DIR : ${CMAKE_SOURCE_DIR}")
message(STATUS "PROJECT_SOURCE_DIR : ${PROJECT_SOURCE_DIR}")
message(STATUS "CMAKE_BINARY_DIR : ${CMAKE_BINARY_DIR}")

include_directories(
    .
    ${PROJECT_SOURCE_DIR}/../../include
    )


add_library(testconfig OBJECT testconfig.c )

add_executable(proxytest proxytest.cpp $<TARGET_OBJECTS:testconfig>)

add_executable(proxytest_mb proxytest.cpp $<TARGET_OBJECTS:testconfig>)
target_compile_definitions(proxytest_mb PUBLIC SMRPROXY_MB)

add_executable(listenertest listenerq.cpp)
