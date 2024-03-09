cmake_minimum_required(VERSION 3.20.0)
project(C-Heap)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O0 -ggdb")
add_executable(C-Heap src/main.c)
