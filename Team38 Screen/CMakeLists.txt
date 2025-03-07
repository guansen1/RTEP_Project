cmake_minimum_required(VERSION 3.15)
project(KeyboardDetection)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")

include_directories(${CMAKE_SOURCE_DIR}/include)

add_executable(my_app
    src/main.cpp
    src/keyboard.cpp
)

target_link_libraries(my_app gpiod)