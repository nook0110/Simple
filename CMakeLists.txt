cmake_minimum_required(VERSION 3.14)
project(SimpleChessEngine LANGUAGES C CXX)

# Option to enable or disable the tests
option(BUILD_TESTS "Build tests" OFF)
option(SCE_TEST_TABLEBASE "Download tablebase fixtures and run tablebase UCI test" ON)
option(SCE_WARNINGS_AS_ERRORS "Treat project warnings as errors" OFF)
option(SCE_SANITIZE "Enable AddressSanitizer and UndefinedBehaviorSanitizer" OFF)
set(SCE_TT_SIZE_MB 640 CACHE STRING "Shared transposition table size in MiB")
set(SCE_EVALFILE "nn-1a298aa575a0.nnue" CACHE STRING "Default NNUE network file")
set(SCE_TARGET_ARCH "native" CACHE STRING "Target architecture for release builds")

if(NOT SCE_TT_SIZE_MB MATCHES "^[1-9][0-9]*$" OR SCE_TT_SIZE_MB LESS 16)
  message(FATAL_ERROR "SCE_TT_SIZE_MB must be a decimal integer of at least 16 MiB for the default bench signature")
endif()

set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
add_compile_definitions(SCE_TT_SIZE_MB=${SCE_TT_SIZE_MB})
add_compile_definitions(SCE_EVALFILE="${SCE_EVALFILE}")

add_compile_options(-Wall -Wextra -Wpedantic)
if(SCE_SANITIZE)
  add_compile_options(-fsanitize=address,undefined -fno-omit-frame-pointer)
  add_link_options(-fsanitize=address,undefined)
endif()
if(CMAKE_BUILD_TYPE STREQUAL "Release")
  add_compile_definitions(NDEBUG)
  add_compile_options(-Ofast -mtune=native -march=${SCE_TARGET_ARCH} -fno-stack-protector)
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

FILE(GLOB CHESS_SOURCES CONFIGURE_DEPENDS Chess/*.cpp Chess/Nnue/*.cpp)
list(APPEND CHESS_SOURCES
    Chess/Nnue/Stockfish/attacks.cpp
    Chess/Nnue/Stockfish/position.cpp
    Chess/Nnue/Stockfish/nnue/nnue_accumulator.cpp
    Chess/Nnue/Stockfish/nnue/features/full_threats.cpp
    Chess/Nnue/Stockfish/nnue/features/pp_3wide.cpp
    Chess/Nnue/Stockfish/nnue/features/half_ka_v2_hm.cpp
)
set_source_files_properties(
    Chess/Nnue/Stockfish/nnue/features/full_threats.cpp
    PROPERTIES COMPILE_OPTIONS
    "$<$<CXX_COMPILER_ID:GNU>:-fconstexpr-ops-limit=500000000>;$<$<CXX_COMPILER_ID:Clang>:-fconstexpr-steps=500000000>"
)
list(REMOVE_ITEM CHESS_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/Chess/main.cpp)

set_property(SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/Chess/Attacks.cpp APPEND PROPERTY COMPILE_OPTIONS
    "$<$<CXX_COMPILER_ID:GNU>:-fconstexpr-ops-limit=100000000>"
    "$<$<CXX_COMPILER_ID:GNU>:-fconstexpr-cache-depth=0>"
    "$<$<CXX_COMPILER_ID:Clang>:-fconstexpr-steps=20000000>"
)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/Chess)

# Single library shared by the main project and the tests
add_library(chess_lib STATIC ${CHESS_SOURCES})
target_link_libraries(chess_lib PUBLIC fathom)

add_executable(${PROJECT_NAME}
    Chess/main.cpp
)

target_link_libraries(${PROJECT_NAME} PRIVATE chess_lib)

if(SCE_WARNINGS_AS_ERRORS)
  target_compile_options(chess_lib PRIVATE -Werror)
  target_compile_options(${PROJECT_NAME} PRIVATE -Werror)
endif()

add_executable(SCE_tbprobe Tools/TablebaseProbe.cpp)
target_link_libraries(SCE_tbprobe PRIVATE chess_lib)
if(SCE_WARNINGS_AS_ERRORS)
  target_compile_options(SCE_tbprobe PRIVATE -Werror)
endif()

add_executable(SCE_tbbench Tools/TablebaseBenchmark.cpp)
target_link_libraries(SCE_tbbench PRIVATE chess_lib)
if(SCE_WARNINGS_AS_ERRORS)
  target_compile_options(SCE_tbbench PRIVATE -Werror)
endif()

# Tests are built only if BUILD_TESTS=ON
if(BUILD_TESTS)
  if(SCE_TEST_TABLEBASE)
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
  endif()
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
        gtest
        chess_lib  # Use the main library
    )
    if(SCE_WARNINGS_AS_ERRORS)
        target_compile_options(SCE_tests PRIVATE -Werror)
    endif()

    # Coverage settings for the tests only
    if(CMAKE_BUILD_TYPE STREQUAL "Coverage" AND
       CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(SCE_tests PRIVATE --coverage -g)
        target_link_options(SCE_tests PRIVATE --coverage)
    endif()

    # Coverage report generation
    add_custom_target(coverage
        COMMAND lcov --capture --directory . --output-file coverage.info --ignore-errors mismatch
        COMMAND lcov --remove coverage.info '/usr/*' '*/Tests/*' --output-file coverage.info
        COMMAND lcov --extract coverage.info '${CMAKE_SOURCE_DIR}/Chess/*' --output-file coverage.info
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
        NAME UciInputHandling
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/Tests/uci_input_test.py
                --engine $<TARGET_FILE:SimpleChessEngine>
    )
    add_test(
        NAME TexelDeterministicQScore
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/Tests/texel_qscore_test.py
                --engine $<TARGET_FILE:SimpleChessEngine>
    )
    if(SCE_TEST_TABLEBASE)
        add_test(
            NAME SyzygyTablebaseUciLifecycle
            COMMAND ${Python3_EXECUTABLE}
                    ${CMAKE_SOURCE_DIR}/Tests/tablebase_uci_test.py
                    --engine $<TARGET_FILE:SimpleChessEngine>
                    --probe $<TARGET_FILE:SCE_tbprobe>
                    --tablebase ${SYZYGY_FIXTURE_DIR}
        )
    endif()
    add_test(
        NAME SyzygySearchRegression
        COMMAND ${Python3_EXECUTABLE}
                ${CMAKE_SOURCE_DIR}/Tests/tablebase_search_test.py
                --engine $<TARGET_FILE:SimpleChessEngine>
    )
    set_tests_properties(SyzygySearchRegression PROPERTIES
        TIMEOUT 60 SKIP_RETURN_CODE 77
    )

    # Copy the test data
    file(GLOB TEST_DATA_FILES ${CMAKE_SOURCE_DIR}/Tests/*.test)
    file(COPY ${TEST_DATA_FILES} DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
endif()

# Dummy target that triggers a rebuild (kept as in the original)
add_custom_target(file_toucher 
    COMMAND ${CMAKE_COMMAND} -E touch_nocreate ${CMAKE_CURRENT_SOURCE_DIR}/Chess/main.cpp)
add_dependencies(${PROJECT_NAME} file_toucher)
