add_executable(mtq_test "")
find_package(Catch2 REQUIRED)
target_link_libraries(mtq_test PRIVATE mtq ssl crypto pthread)
target_include_directories(mtq_test PRIVATE include)
target_sources(mtq_test PRIVATE
    src/main.cpp )
include(CTest)
include(Catch)

catch_discover_tests(mtq_test)