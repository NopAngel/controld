# CMakeLists.txt - Build configuration for controld
cmake_minimum_required(VERSION 3.10)
project(controld VERSION 0.1.0 LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -Wpedantic -ggdb")

# Main daemon
add_executable(controld src/controld.c)

# Control utility (The client)
add_executable(controlctl src/controlctl.c)
