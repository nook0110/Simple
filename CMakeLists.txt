cmake_minimum_required(VERSION 3.10)
project(SimpleChessEngine)

# Добавляем опцию для включения/выключения тестов
option(BUILD_TESTS "Build tests" OFF)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
find_package(glog 0.7.1 REQUIRED)
add_definitions(-DGLOG_USE_GLOG_EXPORT)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_compile_options(-Wall)
if(CMAKE_BUILD_TYPE STREQUAL "Release")
  add_compile_options(-Ofast -mtune=native -march=native -fno-stack-protector -flto -frename-registers -funroll-loops)
  add_link_options(-flto)
endif()

FILE(GLOB CHESS_SOURCES Chess/*.cpp)
list(REMOVE_ITEM CHESS_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/Chess/main.cpp)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/Chess)

# Единая библиотека для основного проекта и тестов
add_library(chess_lib STATIC ${CHESS_SOURCES})

add_executable(${PROJECT_NAME}
    ${CHESS_SOURCES} Chess/main.cpp
)

target_link_libraries(${PROJECT_NAME} PRIVATE chess_lib glog::glog)

# Тесты включаются только если BUILD_TESTS=ON
if(BUILD_TESTS)
    include(FetchContent)
    FetchContent_Declare(
        googletest
        URL https://github.com/google/googletest/archive/03597a01ee50ed33e9dfd640b249b4be3799d395.zip
    )
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)

    FILE(GLOB CHESS_TEST_SOURCES Tests/*.cpp)
    add_executable(SCE_tests ${CHESS_TEST_SOURCES})
    target_link_libraries(SCE_tests 
        PRIVATE 
        gtest_main 
        chess_lib  # Используем основную библиотеку
        glog::glog
    )

    # Настройки покрытия только для тестов
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(SCE_tests PRIVATE --coverage -g)
        target_link_options(SCE_tests PRIVATE --coverage)
    endif()

    # Генерация отчета о покрытии
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

    # Копирование тестовых данных
    file(GLOB TEST_DATA_FILES ${CMAKE_SOURCE_DIR}/Tests/*.test)
    file(COPY ${TEST_DATA_FILES} DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
endif()

# Общие настройки логирования
target_compile_definitions(chess_lib PUBLIC GOOGLE_STRIP_LOG=10)
target_compile_definitions(${PROJECT_NAME} PUBLIC GOOGLE_STRIP_LOG=10)

# Фиктивная цель для триггера пересборки (оставлено как в оригинале)
add_custom_target(file_toucher 
    COMMAND ${CMAKE_COMMAND} -E touch_nocreate ${CMAKE_CURRENT_SOURCE_DIR}/Chess/main.cpp)
add_dependencies(${PROJECT_NAME} file_toucher)
