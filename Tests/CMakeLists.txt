# Required by FetchContent_MakeAvailable
cmake_minimum_required(VERSION 3.14)
project(SimpleChessEngine)

include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/03597a01ee50ed33e9dfd640b249b4be3799d395.zip
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

set(CMAKE_CXX_STANDARD 23)
set(TEST_NAME ${PROJECT_NAME}Tests)
add_executable(${TEST_NAME} test.cpp)
target_link_libraries(${TEST_NAME} gtest_main)

enable_testing()
include(GoogleTest)
gtest_discover_tests(${TEST_NAME})
