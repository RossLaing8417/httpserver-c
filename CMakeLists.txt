cmake_minimum_required(VERSION 3.28)

project(c-http-server C)
set(CMAKE_C_STANDARD 11)

enable_testing()

add_compile_options(-Wall)

add_executable(server src/main.c)

add_subdirectory(src/uri)

# target_include_directories(server PRIVATE ...)

target_link_libraries(server uri)
