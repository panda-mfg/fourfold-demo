cmake_minimum_required(VERSION 3.16)
project(coloring CXX)
find_package(Boost REQUIRED COMPONENTS program_options)
find_package(spdlog REQUIRED)
find_package(fmt REQUIRED)

add_executable(a.out main.cpp coloring.cpp)
target_compile_options(a.out PUBLIC -O2 -Wall)
target_compile_features(a.out PUBLIC cxx_std_20)
target_link_libraries(a.out PRIVATE 
    Boost::boost Boost::program_options
    spdlog::spdlog
    fmt::fmt)
