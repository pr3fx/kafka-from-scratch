cmake_minimum_required(VERSION 3.13)

project(codecrafters-kafka)

file(GLOB_RECURSE SOURCE_FILES src/main.c)

set(CMAKE_C_STANDARD 23) # Enable the C23 standard

add_executable(kafka ${SOURCE_FILES})

target_include_directories(kafka PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src/vendors
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

set_target_properties(kafka PROPERTIES UNITY_BUILD ON)
