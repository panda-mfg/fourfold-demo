cmake_minimum_required(VERSION 3.22)
project(combine CXX)

# Set policy to use new Boost finding behavior
if(POLICY CMP0167)
    cmake_policy(SET CMP0167 NEW)
endif()

# guard against in-source builds
if(${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_BINARY_DIR})
    message(FATAL_ERROR "In-source builds not allowed. Please make a new directory (called a build directory) and run CMake from there. You may need to remove CMakeCache.txt. ")
endif()

set(CMAKE_CXX_STANDARD 23)
include(FetchContent)
FetchContent_Declare(
  googletest
  DOWNLOAD_EXTRACT_TIMESTAMP TRUE
  URL https://github.com/google/googletest/archive/v1.15.2.zip
)
FetchContent_MakeAvailable(googletest)

find_package(Boost REQUIRED COMPONENTS program_options thread)
find_package(spdlog REQUIRED)
find_package(fmt REQUIRED)

add_subdirectory(src)
add_subdirectory(test)
