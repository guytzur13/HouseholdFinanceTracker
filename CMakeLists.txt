cmake_minimum_required(VERSION 3.16)
project(HouseholdFinanceTracker LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/HouseholdFinanceServer)

# All the logic lives in a library, so the app and the tests share the same code.
add_library(finance_core
    ${SRC_DIR}/Money.cpp
    ${SRC_DIR}/Validation.cpp
    ${SRC_DIR}/FinanceManager.cpp
    ${SRC_DIR}/FileStorage.cpp
    ${SRC_DIR}/InputUtils.cpp
    ${SRC_DIR}/ConsoleApp.cpp
)
target_include_directories(finance_core PUBLIC ${SRC_DIR})
if (MSVC)
    target_compile_options(finance_core PRIVATE /W4)
else()
    target_compile_options(finance_core PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_executable(finance_tracker ${SRC_DIR}/main.cpp)
target_link_libraries(finance_tracker PRIVATE finance_core)

# ---- Unit tests (GoogleTest, downloaded automatically) ----
option(BUILD_TESTS "Build the unit tests" ON)
if (BUILD_TESTS)
    include(FetchContent)
    FetchContent_Declare(
        googletest
        URL https://github.com/google/googletest/archive/refs/tags/v1.15.2.tar.gz
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    )
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)

    enable_testing()
    add_executable(finance_tests
        tests/MoneyTests.cpp
        tests/ValidationTests.cpp
        tests/FinanceManagerTests.cpp
        tests/FileStorageTests.cpp
    )
    target_link_libraries(finance_tests PRIVATE finance_core GTest::gtest_main)

    include(GoogleTest)
    gtest_discover_tests(finance_tests)
endif()
