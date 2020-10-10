cmake_minimum_required(VERSION 3.16)
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
if ( ${CMAKE_BUILD_TYPE} STREQUAL "Release")
    message(STATUS "!yeah")

elseif(${CMAKE_BUILD_TYPE} STREQUAL "Debug")
    message(STATUS "oh...")
else ()
    message(FATAL_ERROR "CMAKE_BUILD_TYPE must be Release|Debug. Terminated.")
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "-Wall -Wextra")
set(CMAKE_CXX_FLAGS_DEBUG "-g")
set(CMAKE_CXX_FLAGS_RELEASE "-O3")
project(mtq)


add_executable(mtq "")
target_include_directories(mtq PRIVATE include)

target_link_libraries(mtq pthread ssl crypto)
target_sources(mtq PRIVATE
    src/main.cpp
    PUBLIC
    include/mt_q_classic.h
    include/mt_q_split_lock.h
)
