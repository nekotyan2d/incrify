set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests)

add_library(catch2_local STATIC
    ${CMAKE_SOURCE_DIR}/third_party/catch2/catch_amalgamated.cpp
)
target_include_directories(catch2_local PUBLIC
    ${CMAKE_SOURCE_DIR}/third_party/catch2
)

function(add_unit_test target source)
    add_executable(${target} ${source})
    target_link_libraries(${target} PRIVATE incrify_core catch2_local)
    target_include_directories(${target} PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/third_party
    )
    add_test(NAME ${target} COMMAND ${target})
endfunction()

add_unit_test(test_file_index test_file_index.cpp)
add_unit_test(test_file_indexer test_file_indexer.cpp)
add_unit_test(test_crc32_strategy test_crc32_strategy.cpp)
add_unit_test(test_storage_engine test_storage_engine.cpp)
add_unit_test(test_backup_manager test_backup_manager.cpp)