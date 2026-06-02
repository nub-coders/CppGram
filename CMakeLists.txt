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

set(CPPGRAM_TDLIB_TAG "v1.8.0" CACHE STRING
    "TDLib git tag or commit hash (pin to a release for reproducible builds)")

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

# ---- TDLib via FetchContent ------------------------------------------------
include(FetchContent)

set(CPPGRAM_MAIN_CXX_STANDARD "${CMAKE_CXX_STANDARD}")
set(CPPGRAM_MAIN_CXX_STANDARD_REQUIRED "${CMAKE_CXX_STANDARD_REQUIRED}")
set(CPPGRAM_MAIN_CXX_EXTENSIONS "${CMAKE_CXX_EXTENSIONS}")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(TD_ENABLE_JNI    OFF CACHE BOOL "" FORCE)
set(TD_ENABLE_DOTNET OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
    td
    GIT_REPOSITORY https://github.com/tdlib/td.git
    GIT_TAG        ${CPPGRAM_TDLIB_TAG}
    GIT_SHALLOW    TRUE
)

message(STATUS "Fetching TDLib ${CPPGRAM_TDLIB_TAG} (this may take a while on first build)...")
FetchContent_MakeAvailable(td)

set(CMAKE_CXX_STANDARD "${CPPGRAM_MAIN_CXX_STANDARD}")
set(CMAKE_CXX_STANDARD_REQUIRED "${CPPGRAM_MAIN_CXX_STANDARD_REQUIRED}")
set(CMAKE_CXX_EXTENSIONS "${CPPGRAM_MAIN_CXX_EXTENSIONS}")

# ---- Dependencies ----------------------------------------------------------
find_package(Threads REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(ZLIB    REQUIRED)

find_package(SQLite3 QUIET)

# ---- Core library ----------------------------------------------------------
add_library(cppgram)
add_library(CppGram::cppgram ALIAS cppgram)

target_include_directories(cppgram
    PUBLIC  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
            $<INSTALL_INTERFACE:include>
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

target_sources(cppgram PRIVATE
    src/backend/log.cpp
    src/backend/client.cpp
    src/core/entities.cpp
    src/core/filters.cpp)

target_link_libraries(cppgram
    PUBLIC  Threads::Threads
    PRIVATE Td::TdStatic)

target_compile_definitions(cppgram PRIVATE CPPGRAM_HAVE_TDLIB=1)

if(SQLite3_FOUND)
    target_link_libraries(cppgram PRIVATE SQLite::SQLite3)
    target_compile_definitions(cppgram PRIVATE CPPGRAM_HAVE_SQLITE3=1)
endif()

if(MSVC)
    target_compile_options(cppgram PRIVATE /W4 /permissive-)
else()
    target_compile_options(cppgram PRIVATE -Wall -Wextra -Wpedantic)
endif()

# ---- Subdirectories --------------------------------------------------------
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
