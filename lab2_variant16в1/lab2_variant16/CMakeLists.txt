cmake_minimum_required(VERSION 3.20)
project(lab2_variant16 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(lab2 main.cpp)

if(MSVC)
    target_compile_options(lab2 PRIVATE /W4 /O2)
else()
    find_package(Threads REQUIRED)
    target_link_libraries(lab2 PRIVATE Threads::Threads)
    target_compile_options(lab2 PRIVATE -Wall -Wextra)
endif()
