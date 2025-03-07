cmake_minimum_required(VERSION 3.5)
project(gas_sensor)

set(CMAKE_CXX_STANDARD 11)

include_directories(include)

add_executable(mq2_app src/main.cpp src/ADS1115.cpp src/MQ2Sensor.cpp)
target_link_libraries(mq2_app wiringPi)
