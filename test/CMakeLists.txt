set(INCLUDE_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/../include)
include_directories(${INCLUDE_DIRECTORY})
link_libraries(Threads::Threads)

add_executable(test_hash_table test_hash_table.cpp)