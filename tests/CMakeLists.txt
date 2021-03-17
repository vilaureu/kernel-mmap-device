cmake_minimum_required(VERSION 3.19.0)
project(KernelMmapDeviceTests)

SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra")

add_executable(tests tests.c)
