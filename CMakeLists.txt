cmake_minimum_required(VERSION 3.21)
project(CppGram
    VERSION 0.1.0
    DESCRIPTION "Modern C++20 Telegram client framework"
    LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(CPPGRAM_BUILD_TESTS      "Build unit tests"   ON)
option(CPPGRAM_BUILD_EXAMPLES   "Build examples"     ON)
option(CPPGRAM_BUILD_BENCHMARKS "Build benchmarks"   OFF)
option(CPPGRAM_USE_TDLIB        "Link real TDLib"    OFF) # OFF until Phase 2

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

# ---- Core library --------------------------------------------------------
add_library(cppgram)
add_library(CppGram::cppgram ALIAS cppgram)

target_include_directories(cppgram
    PUBLIC  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
            $<INSTALL_INTERFACE:include>
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

target_sources(cppgram PRIVATE
    src/core/log.cpp
    src/core/client.cpp
    src/entities/entities.cpp)

# SQLite (Phase 3 storage) — vendored or system
find_package(SQLite3 QUIET)
if(SQLite3_FOUND)
    target_link_libraries(cppgram PRIVATE SQLite::SQLite3)
    target_compile_definitions(cppgram PRIVATE CPPGRAM_HAVE_SQLITE3=1)
endif()

find_package(Threads REQUIRED)
target_link_libraries(cppgram PUBLIC Threads::Threads)

if(CPPGRAM_USE_TDLIB)
    find_package(TDLib REQUIRED)               # cmake/FindTDLib.cmake (Phase 2)
    target_link_libraries(cppgram PRIVATE Td::TdStatic)
    target_compile_definitions(cppgram PRIVATE CPPGRAM_HAVE_TDLIB=1)
endif()

if(MSVC)
    target_compile_options(cppgram PRIVATE /W4 /permissive-)
else()
    target_compile_options(cppgram PRIVATE -Wall -Wextra -Wpedantic)
endif()

# ---- Subdirectories ------------------------------------------------------
if(CPPGRAM_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()
if(CPPGRAM_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
if(CPPGRAM_BUILD_BENCHMARKS)
    add_subdirectory(benchmarks)
endif()