#proxies
cmake_minimum_required(VERSION 3.20)

project(proxies)

add_subdirectory(test)
add_subdirectory(test/proxytest)

if (CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O3 -std=gnu17 -ggdb")
    set(CMAKE_CXX_FLAGS, "${CMAKE_CXX_FLAGS} -03 -std=c++20 -fconcepts -ggdb")
else ()
    message(FATAL_ERROR "unsupported system type")
endif ()

message(STATUS "CMAKE_SOURCE_DIR : ${CMAKE_SOURCE_DIR}")
message(STATUS "PROJECT_SOURCE_DIR : ${PROJECT_SOURCE_DIR}")

# include_directories(AFTER ${PROJECT_SOURCE_DIR}/include)

install(FILES
    proxy/proxy.h
    smrproxy/epoch.h
    smrproxy/smrproxy.h
    arcproxy/arcproxy.h
    sharedproxy/sharedproxy.h
    DESTINATION .
    )