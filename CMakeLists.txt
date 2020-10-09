cmake_minimum_required(VERSION 3.1)

set(CMAKE_CXX_EXTENSIONS OFF)
option(TOM_KV_TEST "Tests for tom-kv" OFF)

set(CMAKE_CXX_COMPILER g++)
set(CMAKE_CXX_STANDARD 17)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# find_package(Boost REQUIRED)
find_package(Threads REQUIRED)

if (TOM_KV_TEST)
    message(STATUS "Adding test subdirectory")
    add_subdirectory(test)
endif()