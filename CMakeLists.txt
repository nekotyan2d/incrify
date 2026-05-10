cmake_minimum_required(VERSION 3.15)
project(Incrify VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(incrify
    src/main.cpp
    src/models/FileIndex.cpp
    src/core/CRC32Strategy.cpp
    src/core/FileIndexer.cpp
    src/core/StorageEngine.cpp
    src/core/BackupManager.cpp
    src/cli/CLI.cpp
)

target_include_directories(incrify PRIVATE
    include
    ${CMAKE_SOURCE_DIR}/third_party
)

target_compile_options(incrify PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall -Wextra -Wpedantic>
    $<$<CXX_COMPILER_ID:MSVC>:/W4>
)