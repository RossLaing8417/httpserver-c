cmake_minimum_required(VERSION 3.28)

project(c-http-server C)
set(CMAKE_C_STANDARD 11)

add_compile_options(-Wall)

add_executable(server src/main.c)

# target_include_directories(server PRIVATE ...)

target_link_libraries(server)
