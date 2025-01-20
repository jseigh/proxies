#proxies test
cmake_minimum_required(VERSION 3.20)

project(proxy_test)

if (CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O3 -std=gnu2x -ggdb")
    set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS} -ggdb")
    set(CMAKE_CXX_FLAGS "-O3 -std=c++20 -fconcepts -ggdb -fconcepts-diagnostics-depth=4")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS} -ggdb")
else ()
    message(FATAL_ERROR "unsupported system type")
endif ()


message(STATUS "CMAKE_SOURCE_DIR : ${CMAKE_SOURCE_DIR}")
message(STATUS "PROJECT_SOURCE_DIR : ${PROJECT_SOURCE_DIR}")
message(STATUS "CMAKE_BINARY_DIR : ${CMAKE_BINARY_DIR}")
message(STATUS "CMAKE_CXX_SOURCE_FILE_EXTENSIONS : ${CMAKE_CXX_SOURCE_FILE_EXTENSIONS}")
message(STATUS "CMAKE_CXX_FLAGS : ${CMAKE_CXX_FLAGS}")

include_directories(
    .
    ${PROJECT_SOURCE_DIR}/../include
    ${PROJECT_SOURCE_DIR}/../smrproxy
    )


add_executable(epoch_test epoch_test.cpp)

add_executable(epoch_test2 epoch_test.cpp)
target_compile_options(epoch_test2 PUBLIC
    -DEPOCH_NO_WRAP
    )


