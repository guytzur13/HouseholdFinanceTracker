cmake_minimum_required(VERSION 3.16)
project(HouseholdFinanceTracker VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# The source files contain Hebrew text; tell the Microsoft compiler they are UTF-8.
if (MSVC)
    add_compile_options(/utf-8)
endif()

set(SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/HouseholdFinanceServer)

# ---- Core library: business logic and storage, shared by every front end and the tests ----
add_library(finance_core
    ${SRC_DIR}/Money.cpp
    ${SRC_DIR}/Validation.cpp
    ${SRC_DIR}/FinanceManager.cpp
    ${SRC_DIR}/FileStorage.cpp
)
target_include_directories(finance_core PUBLIC ${SRC_DIR})
if (MSVC)
    target_compile_options(finance_core PRIVATE /W4)
else()
    target_compile_options(finance_core PRIVATE -Wall -Wextra -Wpedantic)
endif()

# ---- Console app ----
add_executable(finance_tracker
    ${SRC_DIR}/main.cpp
    ${SRC_DIR}/InputUtils.cpp
    ${SRC_DIR}/ConsoleApp.cpp
)
target_link_libraries(finance_tracker PRIVATE finance_core)

# ---- Desktop app (Qt 6). Built only when Qt is installed. ----
option(BUILD_GUI "Build the Qt desktop app" ON)
if (BUILD_GUI)
    if (APPLE)
        # Where Homebrew installs Qt (Apple silicon and Intel Macs)
        list(APPEND CMAKE_PREFIX_PATH /opt/homebrew/opt/qt /usr/local/opt/qt)
    endif()
    find_package(Qt6 QUIET COMPONENTS Widgets)
    if (Qt6_FOUND)
        qt_add_executable(HouseholdFinance
            gui/main.cpp
            gui/MainWindow.cpp
            gui/TransactionDialog.cpp
        )
        set_target_properties(HouseholdFinance PROPERTIES AUTOMOC ON)  # Qt's code generator for Q_OBJECT
        target_include_directories(HouseholdFinance PRIVATE gui)
        target_link_libraries(HouseholdFinance PRIVATE finance_core Qt6::Widgets)
        set_target_properties(HouseholdFinance PROPERTIES
            WIN32_EXECUTABLE ON          # no console window on Windows
            MACOSX_BUNDLE ON             # a real .app on macOS
            MACOSX_BUNDLE_BUNDLE_NAME "HouseholdFinance"
            MACOSX_BUNDLE_GUI_IDENTIFIER "com.guytzur.householdfinance"
            MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
            MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION}
        )
        message(STATUS "Qt ${Qt6_VERSION} found: building the desktop app")
    else()
        message(STATUS "Qt 6 not found: skipping the desktop app (install Qt to build it)")
    endif()
endif()

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
