cmake_minimum_required(VERSION 3.10)
project(Incrify)

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

target_include_directories(incrify PRIVATE include)