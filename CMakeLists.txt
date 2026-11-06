cmake_minimum_required(VERSION 3.13)

project(codecrafters-kafka)

file(GLOB_RECURSE SOURCE_FILES src/main.c)

set(CMAKE_C_STANDARD 23) # Enable the C23 standard
set(CMAKE_UNITY_BUILD ON)

add_executable(kafka ${SOURCE_FILES})
