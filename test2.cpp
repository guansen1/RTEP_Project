#include <iostream>
#include <thread>
#include <chrono>
#include "gpio/gpio.h"
#include "pir/pir.h"
#include "dht/dht.h"
#include "display/i2c_display.h"
#include "display/i2c_handle.h"
#include "buzzer/buzzer.h"
#include "keyboard/keyboard.h"  // 引入你的键盘模块

int main() {
    std::cout << "System Starting..." << std::endl;
    
    // 初始化 I2C 显示模块（OLED）
    I2cDisplay::getInstance().init();
    
    // 初始化 GPIO 模块
    GPIO gpio;
    gpio.gpio_init();

    RPI_PWM pwm;
    Buzzer buzzer(pwm);
    ActiveKeyboardScanner keyboardScanner(gpio);
    // PIR 人体红外检测模块事件处理器
    PIREventHandler pirHandler(gpio, buzzer);
    gpio.registerCallback(PIR_IO, &pirHandler);

    // 初始化显示控制器并注册 PIR 事件回调
    I2cDisplayHandle displayHandle(buzzer);
    gpio.registerCallback(PIR_IO, &displayHandle);

    // 启动 GPIO 事件监听线程
    gpio.start();

    // 初始化 DHT11 温湿度传感器
    DHT11 dht11(gpio);
    dht11.registerCallback([&displayHandle](const DHTReading &reading) {
        displayHandle.handleDHT(reading.temp_celsius, reading.humidity);
    });
    dht11.start();

    // ✅ 替换虚拟键盘：初始化 4x4 硬件矩阵键盘
   
    keyboardScanner.setKeyCallback([&displayHandle](char key) {
        displayHandle.handleKeyPress(key);  // 触发显示处理逻辑
        std::cout << "[HardwareKey] " << key << " pressed" << std::endl;
    });
    keyboardScanner.start();  // 启动键盘扫描线程

    // 主循环保持运行
    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    gpio.stop();  // 程序实际中不会到这，但结构完整性需要
    return 0;
}
