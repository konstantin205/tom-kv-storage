# * MIT License
# *
# * Copyright (c) 2020 Konstantin Boyarinov
# *
# * Permission is hereby granted, free of charge, to any person obtaining a copy
# * of this software and associated documentation files (the "Software"), to deal
# * in the Software without restriction, including without limitation the rights
# * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# * copies of the Software, and to permit persons to whom the Software is
# * furnished to do so, subject to the following conditions:
# *
# * The above copyright notice and this permission notice shall be included in all
# * copies or substantial portions of the Software.
# *
# * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# * SOFTWARE.

cmake_minimum_required(VERSION 3.1)

set(CMAKE_CXX_EXTENSIONS OFF)
option(TOMKV_TEST "Tests for tomkv" OFF)
option(TOMKV_BENCH "Performance benchmarks for tomkv" OFF)

if(NOT CMAKE_CXX_STANDARD)
    set(CMAKE_CXX_STANDARD 17)
endif()

if(NOT CMAKE_BUILD_TYPE)
    message(STATUS "CMAKE_BUILD_TYPE is not specified: using Release as a default")
    set(CMAKE_BUILD_TYPE Release)
endif()

find_package(Boost COMPONENTS filesystem REQUIRED)
find_package(Threads REQUIRED)

set(INCLUDE_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include)
include_directories(${INCLUDE_DIRECTORY})
include_directories(${Boost_INCLUDE_DIRS})

link_libraries(Threads::Threads)
link_libraries(${Boost_LIBRARIES})

if (TOMKV_TEST)
    message(STATUS "Adding test subdirectory")
    add_subdirectory(test)
endif()

if (TOMKV_BENCH)
    message(STATUS "Adding bench subdirectory")
    add_subdirectory(bench)
endif()
