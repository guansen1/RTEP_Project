# 设置 CMake 的最低版本要求
cmake_minimum_required(VERSION 3.10)

# 设置项目名称
project(AlarmSystem)

# 设置 C++ 标准
set(CMAKE_CXX_STANDARD 17)

# 设置编译选项
set(CMAKE_CXX_FLAGS "-Wall -O2 -pthread")

# 查找 gpiod 库（确保已经安装 libgpiod-dev）
find_package(PkgConfig REQUIRED)
pkg_check_modules(GPIOD REQUIRED libgpiod)

# 将源文件添加到可执行文件
add_executable(alarm_system main.cpp gpio/gpio.cpp dht/dht.cpp pir/pir.cpp)

# 添加 libgpiod 库链接
target_link_libraries(alarm_system ${GPIOD_LIBRARIES})

# 包含头文件目录
target_include_directories(alarm_system PRIVATE ${GPIOD_INCLUDE_DIRS})
include_directories(
    ${CMAKE_SOURCE_DIR}/dht
    ${CMAKE_SOURCE_DIR}/gpio
    ${CMAKE_SOURCE_DIR}/pir
    )
# 如果有其他需要链接的库，可以在此处继续添加
