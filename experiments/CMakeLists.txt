cmake_minimum_required(VERSION 3.12)

project (main VERSION 1.0)

find_package(OpenMP)

add_executable(script script.cpp)

target_link_libraries(script OpenMP::OpenMP_CXX) 
