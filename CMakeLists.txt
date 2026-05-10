cmake_minimum_required(VERSION 3.15)
project(Incrify VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(MSVC)
    set(COMPILE_OPTIONS /W4)
else()
    set(COMPILE_OPTIONS -Wall -Wextra -Wpedantic)
endif()

add_library(incrify_core STATIC
    src/models/FileIndex.cpp
    src/core/CRC32Strategy.cpp
    src/core/FileIndexer.cpp
    src/core/StorageEngine.cpp
    src/core/BackupManager.cpp
)

target_include_directories(incrify_core PUBLIC
    include
    ${CMAKE_SOURCE_DIR}/third_party
)

target_compile_options(incrify_core PRIVATE ${COMPILE_OPTIONS})

add_executable(incrify
    src/main.cpp
    src/cli/CLI.cpp
)

target_link_libraries(incrify PRIVATE incrify_core)
target_compile_options(incrify PRIVATE ${COMPILE_OPTIONS})

enable_testing()

option(BUILD_TESTS "Build tests" ON)
if(BUILD_TESTS)
    add_subdirectory(tests)
endif()