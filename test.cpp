#include <iostream>
#include <thread>
#include <chrono>
#include "gpio/gpio.h"
#include "pir/pir.h"
#include "dht/dht.h"
#include "display/i2c_display.h"
#include "display/i2c_handle.h"
#include "buzzer/buzzer.h"

// 使用电脑键盘模拟矩阵键盘输入
void keyboardLoop(I2cDisplayHandle &displayHandle) {
    char ch;
    std::cout << "Please enter password digits using your computer keyboard:" << std::endl;
    while (std::cin >> ch) {
        displayHandle.handleKeyPress(ch);
        std::cout << "*" << std::flush;
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

int main() {
    std::cout << "System Starting..." << std::endl;
    
    // 初始化 I2C 显示模块（OLED）
    I2cDisplay::getInstance().init();
    
    // 初始化 GPIO 模块
    GPIO gpio;
    gpio.gpio_init();
        // 创建 Buzzer 实例（传入 GPIO 和蜂鸣器引脚，例如 BUZZER_IO）
    Buzzer buzzer(gpio, BUZZER_IO);
    // 注册 PIR 事件处理器（用于日志输出）
    PIREventHandler pirHandler(gpio,buzzer);
    gpio.registerCallback(PIR_IO, &pirHandler);
    

    
    // 创建 I2cDisplayHandle 实例，并注入 buzzer 对象
    I2cDisplayHandle displayHandle(buzzer);
    gpio.registerCallback(PIR_IO, &displayHandle);
    
    // 启动 GPIO 事件监听线程
    gpio.start();
    
    // 初始化 DHT11 温湿度传感器，并注册回调更新显示（真实数据）
    DHT11 dht11(gpio);
    dht11.registerCallback([&displayHandle](const DHTReading &reading) {
        displayHandle.handleDHT(reading.temp_celsius, reading.humidity);
    });
    dht11.start();
    
    // 启动电脑键盘输入线程，模拟矩阵键盘按键输入
    std::thread kbThread(keyboardLoop, std::ref(displayHandle));
    kbThread.detach();
    
    // 主循环保持运行
    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    
    gpio.stop();
    return 0;
}
