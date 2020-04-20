cmake_minimum_required(VERSION 3.10)

project(Bekos ASM C CXX)
set(CMAKE_CXX_STANDARD 11)

add_subdirectory(kernel/src)
add_subdirectory(userspace)


