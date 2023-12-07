cmake_minimum_required(VERSION 3.16)
project(Helloworld)

# settings for project
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# includes
include_directories(../include/)

# add c/cpp
add_library(Helloworld SHARED main.c)
