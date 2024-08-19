cmake_minimum_required(VERSION 3.10)

project (main VERSION 1.0)

set(CMAKE_TOOLCHAIN_FILE ${CMAKE_CURRENT_SOURCE_DIR}/vcpkg/scripts/buildsystems/vcpkg.cmake
  CACHE STRING "Vcpkg toolchain file")

find_package(SEAL)
find_package(OpenMP)

add_executable(main-base main-base.cpp server.cpp client.cpp utils.cpp)
add_executable(main2 main2.cpp server.cpp client.cpp utils.cpp)
add_executable(main-psi main-psi.cpp server.cpp client.cpp utils.cpp)
add_executable(main-psi-large main-psi-large.cpp server.cpp client.cpp utils.cpp)

target_link_libraries(main-base SEAL::seal OpenMP::OpenMP_CXX)
target_link_libraries(main2 SEAL::seal OpenMP::OpenMP_CXX)
target_link_libraries(main-psi SEAL::seal OpenMP::OpenMP_CXX)
target_link_libraries(main-psi-large SEAL::seal OpenMP::OpenMP_CXX)

set(CMAKE_CXX_FLAGS "-g")
