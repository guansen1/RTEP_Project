# 设置 CMake 的最低版本要求
cmake_minimum_required(VERSION 3.10)

# 设置项目名称
project(AlarmSystem)

# 设置 C++ 标准
set(CMAKE_CXX_STANDARD 17)

# 设置编译选项
set(CMAKE_CXX_FLAGS "-Wall -O2 -pthread")

# 启用测试选项
option(BUILD_TESTS "构建测试" OFF)
option(ENABLE_SMOOTHING_TEST "启用数据平滑测试" OFF)
option(ENABLE_HARDWARE_TEST "启用实际硬件测试" OFF)

if(ENABLE_SMOOTHING_TEST)
    add_definitions(-DTEST_SMOOTHING_ENABLED)
endif()

if(ENABLE_HARDWARE_TEST)
    add_definitions(-DHARDWARE_TEST_ENABLED)
endif()

# 查找 gpiod 库
find_package(PkgConfig REQUIRED)
pkg_check_modules(GPIOD REQUIRED libgpiod)

# 手动查找 libi2c
set(I2C_LIB "/usr/lib/aarch64-linux-gnu/libi2c.so")
set(I2C_INCLUDE_DIR "/usr/include/linux")

# 包含目录
include_directories(
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/dht
    ${CMAKE_SOURCE_DIR}/gpio  
    ${CMAKE_SOURCE_DIR}/pir
    ${CMAKE_SOURCE_DIR}/display
)

# 添加库文件
add_library(gpio_lib gpio/gpio.cpp)
target_link_libraries(gpio_lib ${GPIOD_LIBRARIES})

add_library(dht_lib dht/dht.cpp)
target_link_libraries(dht_lib gpio_lib)

add_library(pir_lib pir/pir.cpp)
target_link_libraries(pir_lib gpio_lib)

add_library(display_lib 
    display/i2c_display.cpp
    display/i2c_handle.cpp
)
target_link_libraries(display_lib ${I2C_LIB})

# 添加主可执行文件
add_executable(alarm_system main.cpp)
target_link_libraries(alarm_system 
    gpio_lib
    dht_lib
    pir_lib
    display_lib
)

# 测试配置
if(BUILD_TESTS)
    # 添加GoogleTest
    include(FetchContent)
    FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG release-1.12.1
    )
    # 对于cmake 3.14+，可以使用下面的简化形式
    FetchContent_MakeAvailable(googletest)
    
    # 启用测试
    enable_testing()
    
    # 添加DHT11单元测试
    add_executable(dht11_test tests/dht11_test.cpp)
    target_link_libraries(dht11_test
        gtest
        gtest_main
        gmock
        dht_lib
        gpio_lib
        pthread
        ${GPIOD_LIBRARIES}
    )
    add_test(NAME DHT11Test COMMAND dht11_test)
    
    # 以下是可选的其他传感器测试
    # 例如PIR传感器测试
    # add_executable(pir_test tests/pir_test.cpp)
    # target_link_libraries(pir_test gtest gtest_main gmock pir_lib gpio_lib pthread ${GPIOD_LIBRARIES})
    # add_test(NAME PIRTest COMMAND pir_test)
endif()