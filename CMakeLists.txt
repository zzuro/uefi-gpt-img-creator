cmake_minimum_required(VERSION 3.15)
project(gpt-img-creator)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR})

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -Wpedantic -O3")

file(GLOB_RECURSE SOURCES "src/*.c")
include_directories("include")


add_executable(${PROJECT_NAME} main.c ${SOURCES})

add_subdirectory(efi_hello)