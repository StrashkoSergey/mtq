cmake_minimum_required(VERSION 3.16)
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
if ( ${CMAKE_BUILD_TYPE} STREQUAL "Release")
    message(STATUS "!yeah")

elseif(${CMAKE_BUILD_TYPE} STREQUAL "Debug")
    message(STATUS "oh...")
else ()
    message(FATAL_ERROR "CMAKE_BUILD_TYPE must be Release|Debug. Terminated.")
endif()

if(CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUCXX)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Werror -pedantic")
	set(CMAKE_CXX_CLANG_TIDY "clang-tidy")
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "-Wall -Wextra")
set(CMAKE_CXX_FLAGS_DEBUG "-g")
set(CMAKE_CXX_FLAGS_RELEASE "-O3")
if (APPLE)
    link_directories("/usr/local/opt/openssl@1.1/lib" )
    include_directories("/usr/local/opt/openssl@1.1/include")
endif()

project(mtq)


add_executable(mtq "")
set_property(TARGET mtq PROPERTY INTERPROCEDURAL_OPTIMIZATION True)
target_include_directories(mtq PRIVATE include)

target_link_libraries(mtq pthread ssl crypto)
target_sources(mtq PRIVATE
    src/main.cpp
    PUBLIC
    include/mt_q_classic.h
    include/mt_q_split_lock.h
)
