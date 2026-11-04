cmake_minimum_required(VERSION 3.14)
project(SimpleChessEngine LANGUAGES C CXX)

# Добавляем опцию для включения/выключения тестов
option(BUILD_TESTS "Build tests" OFF)
set(SCE_TT_SIZE_MB 640 CACHE STRING "Shared transposition table size in MiB")

set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
add_compile_definitions(SCE_TT_SIZE_MB=${SCE_TT_SIZE_MB})

add_compile_options(-Wall -Wextra -Wpedantic)
if(CMAKE_BUILD_TYPE STREQUAL "Release")
  add_compile_definitions(NDEBUG)
  add_compile_options(-Ofast -mtune=native -march=native -fno-stack-protector)
  add_compile_options($<$<COMPILE_LANGUAGE:CXX>:-flto=auto>
                      $<$<COMPILE_LANGUAGE:CXX>:-frename-registers>)
  add_compile_options($<$<COMPILE_LANGUAGE:CXX>:-fno-exceptions>
                      $<$<COMPILE_LANGUAGE:CXX>:-fno-rtti>)
  add_link_options(-flto)
endif()

include(FetchContent)
find_package(Threads REQUIRED)
FetchContent_Declare(
    fathom
    GIT_REPOSITORY https://github.com/jdart1/Fathom.git
    GIT_TAG c9c6fef0dddc05d2e242c183acf5833149ab676d
)
FetchContent_MakeAvailable(fathom)
add_library(fathom STATIC ${fathom_SOURCE_DIR}/src/tbprobe.c)
set_target_properties(fathom PROPERTIES C_STANDARD 11 C_STANDARD_REQUIRED YES)
target_include_directories(fathom PUBLIC ${fathom_SOURCE_DIR}/src)
target_compile_definitions(fathom PRIVATE TB_NO_HELPER_API)
target_link_libraries(fathom PRIVATE Threads::Threads)

FILE(GLOB CHESS_SOURCES CONFIGURE_DEPENDS Chess/*.cpp)
list(REMOVE_ITEM CHESS_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/Chess/main.cpp)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/Chess)

# Единая библиотека для основного проекта и тестов
add_library(chess_lib STATIC ${CHESS_SOURCES})
target_link_libraries(chess_lib PUBLIC fathom)

add_executable(${PROJECT_NAME}
    Chess/main.cpp
)

target_link_libraries(${PROJECT_NAME} PRIVATE chess_lib)

add_executable(SCE_tbprobe Tools/TablebaseProbe.cpp)
target_link_libraries(SCE_tbprobe PRIVATE chess_lib)

add_executable(SCE_tbbench Tools/TablebaseBenchmark.cpp)
target_link_libraries(SCE_tbbench PRIVATE chess_lib)

# Тесты включаются только если BUILD_TESTS=ON
if(BUILD_TESTS)
    set(SYZYGY_FIXTURE_DIR ${CMAKE_CURRENT_BINARY_DIR}/syzygy-fixture)
    file(MAKE_DIRECTORY ${SYZYGY_FIXTURE_DIR})
    file(DOWNLOAD
        https://tablebase.lichess.ovh/tables/standard/3-4-5-wdl/KQvK.rtbw
        ${SYZYGY_FIXTURE_DIR}/KQvK.rtbw
        EXPECTED_HASH SHA256=517667dff787162dbb1ed9d5d6484d30ee854e686ee0675c08d99ecf045d2d50
        TLS_VERIFY ON
    )
    file(DOWNLOAD
        https://tablebase.lichess.ovh/tables/standard/3-4-5-dtz/KQvK.rtbz
        ${SYZYGY_FIXTURE_DIR}/KQvK.rtbz
        EXPECTED_HASH SHA256=71ea9444fa5bd42897d781a0c356975ea6f23e0f65a4254e470897031c161c8c
        TLS_VERIFY ON
    )
    FetchContent_Declare(
        googletest
        URL https://github.com/google/googletest/archive/03597a01ee50ed33e9dfd640b249b4be3799d395.zip
    )
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)

    FILE(GLOB CHESS_TEST_SOURCES CONFIGURE_DEPENDS Tests/*.cpp)
    add_executable(SCE_tests ${CHESS_TEST_SOURCES})
    add_dependencies(SCE_tests ${PROJECT_NAME})
    add_dependencies(SCE_tests SCE_tbprobe)
    add_dependencies(SCE_tests SCE_tbbench)
    target_link_libraries(SCE_tests 
        PRIVATE 
        gtest_main 
        chess_lib  # Используем основную библиотеку
    )

    # Настройки покрытия только для тестов
    if(CMAKE_BUILD_TYPE STREQUAL "Coverage" AND
       CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
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

    find_package(Python3 COMPONENTS Interpreter REQUIRED)
    add_test(
        NAME LazySmpUciLifecycle
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/Tests/uci_lazy_smp_test.py
                --engine $<TARGET_FILE:SimpleChessEngine>
    )
    add_test(
        NAME TexelDeterministicQScore
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/Tests/texel_qscore_test.py
                --engine $<TARGET_FILE:SimpleChessEngine>
    )
    add_test(
        NAME SyzygyTablebaseUciLifecycle
        COMMAND ${Python3_EXECUTABLE}
                ${CMAKE_SOURCE_DIR}/Tests/tablebase_uci_test.py
                --engine $<TARGET_FILE:SimpleChessEngine>
                --probe $<TARGET_FILE:SCE_tbprobe>
                --tablebase ${SYZYGY_FIXTURE_DIR}
    )

    # Копирование тестовых данных
    file(GLOB TEST_DATA_FILES ${CMAKE_SOURCE_DIR}/Tests/*.test)
    file(COPY ${TEST_DATA_FILES} DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
endif()

# Фиктивная цель для триггера пересборки (оставлено как в оригинале)
add_custom_target(file_toucher 
    COMMAND ${CMAKE_COMMAND} -E touch_nocreate ${CMAKE_CURRENT_SOURCE_DIR}/Chess/main.cpp)
add_dependencies(${PROJECT_NAME} file_toucher)
