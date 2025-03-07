# 设置 CMake 的最低版本要求
cmake_minimum_required(VERSION 3.10)

# 设置项目名称
project(AlarmSystem)

# 设置 C++ 标准
set(CMAKE_CXX_STANDARD 17)

# 设置编译选项
set(CMAKE_CXX_FLAGS "-Wall -O2 -pthread")

# 查找 gpiod 库
find_package(PkgConfig REQUIRED)
pkg_check_modules(GPIOD REQUIRED libgpiod)

# 手动查找 libi2c
set(I2C_LIB "/usr/lib/aarch64-linux-gnu/libi2c.so")
set(I2C_INCLUDE_DIR "/usr/include/linux")


include_directories(
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/dht
    ${CMAKE_SOURCE_DIR}/gpio  # 确保包含 gpio 目录
    ${CMAKE_SOURCE_DIR}/pir
    ${CMAKE_SOURCE_DIR}/display
)


# 添加可执行文件
add_executable(alarm_system 
    main.cpp 
    gpio/gpio.cpp 
    dht/dht.cpp 
    pir/pir.cpp 
    display/i2c_display.cpp
    display/i2c_handle.cpp
)

# 链接库
target_link_libraries(alarm_system ${GPIOD_LIBRARIES} ${I2C_LIB})
