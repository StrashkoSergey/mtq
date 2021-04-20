cmake_minimum_required(VERSION 3.16)
set(CMAKE_CXX_STANDARD 17)
project(mtq)


if(CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUCXX)
    message("GCC")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Werror -pedantic")
	set(CMAKE_CXX_CLANG_TIDY "clang-tidy")
endif()
add_subdirectory(lib)

# target_compile_definitions(${TARGET} PRIVATE
#     TEST_DATA_PATH="${PROJECT_SOURCE_DIR}/data")

# set environment variable to ON if you want to enable testing
if (BUILD_TESTING)
    enable_testing()
    add_subdirectory(test)
endif()
