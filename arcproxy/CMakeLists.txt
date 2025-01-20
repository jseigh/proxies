cmake_minimum_required(VERSION 3.20)

project(arcproxy)

if (CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set(platform "linux")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O3 -std=gnu17 -ggdb")
    set(CMAKE_CXX_FLAGS, "${CMAKE_CXX_FLAGS} -03 -std=c++20 -fconcepts -ggdb")
elseif (CMAKE_SYSTEM_NAME STREQUAL "Windows")
    set(platform "windows")
    message(FATAL_ERROR "windows does not support C11/C17")
else ()
    message(FATAL_ERROR "unsupported system type")
endif ()

message(STATUS "CMAKE_SOURCE_DIR : ${CMAKE_SOURCE_DIR}")
message(STATUS "PROJECT_SOURCE_DIR : ${PROJECT_SOURCE_DIR}")
message(STATUS "CMAKE_BINARY_DIR : ${CMAKE_BINARY_DIR}")

include_directories( ../include)

# set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")




# add_library(smrproxy STATIC
#     src/smrproxy.c
#     src/smrqueue.c
#     src/platform/${platform}/membarrier.c
#     src/platform/${platform}/smr_util.c
# )

# target_include_directories(smrproxy PUBLIC
#     include
#     src
# )

# install(TARGETS smrproxy ARCHIVE DESTINATION lib)
# install(FILES include/smrproxy.h DESTINATION include)


