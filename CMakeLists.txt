cmake_minimum_required(VERSION 3.15)
project(minidb VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Threads REQUIRED)

add_library(minidb_lib
    src/DiskManager.cpp
    src/BTree.cpp
    src/WAL.cpp
    src/Database.cpp
    src/QueryParser.cpp
)
target_include_directories(minidb_lib PUBLIC include)
target_link_libraries(minidb_lib Threads::Threads)

add_executable(minidb src/main.cpp)
target_link_libraries(minidb minidb_lib)

add_executable(benchmark src/benchmark.cpp)
target_link_libraries(benchmark minidb_lib)

include(FetchContent)
FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.tar.gz
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

enable_testing()
add_executable(run_tests tests/test_minidb.cpp)
target_link_libraries(run_tests minidb_lib GTest::gtest_main)
target_include_directories(run_tests PRIVATE include)
add_test(NAME MiniDBTests COMMAND run_tests)
