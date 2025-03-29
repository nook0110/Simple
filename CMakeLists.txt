cmake_minimum_required(VERSION 3.10)
project(SimpleChessEngine)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

FILE(GLOB CHESS_SOURCES Chess/*.cpp)
list(REMOVE_ITEM CHESS_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/Chess/main.cpp)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}) 


add_library(chess_lib ${CHESS_SOURCES})
add_library(chess_lib_tests ${CHESS_SOURCES})
add_executable(${PROJECT_NAME}
    ${CHESS_SOURCES} Chess/main.cpp
)

target_link_libraries(${PROJECT_NAME} chess_lib)

include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/03597a01ee50ed33e9dfd640b249b4be3799d395.zip
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

set(CMAKE_CXX_STANDARD 20)
FILE(GLOB CHESS_TEST_SOURCES Tests/*.cpp)
add_executable(SCE_tests ${CHESS_TEST_SOURCES})
target_link_libraries(SCE_tests gtest_main chess_lib_tests)


if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  target_compile_options(SCE_tests PRIVATE --coverage -g)
  target_link_options(SCE_tests PRIVATE --coverage)
    target_compile_options(chess_lib_tests PRIVATE --coverage -g)
  target_link_options(chess_lib_tests PRIVATE --coverage)
endif()

add_custom_target(coverage
    COMMAND lcov --capture --directory . --output-file coverage.info --ignore-errors mismatch
    COMMAND lcov --remove coverage.info '/usr/*' '*/Tests/*' --output-file coverage.info
    COMMAND lcov --extract coverage.info '${CMAKE_SOURCE_DIR}/source/*' --output-file coverage.info
    COMMAND genhtml coverage.info --output-directory ${CMAKE_SOURCE_DIR}/docs/coverage_report
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "Generating code coverage report"
)

enable_testing()
include(GoogleTest)
gtest_discover_tests(SCE_tests)
