#include <iostream>
#include <thread>
#include <chrono>
#include "gpio/gpio.h"
#include "pir/pir.h"
#include "dht/dht.h"
#include "display/i2c_display.h"
#include "i2c_handle.h"
#include "keyboard.h"

int main() {
    std::cout << "系统启动！" << std::endl;

    // 初始化 I2C 显示模块（SSD1306）
    I2cDisplay::getInstance().init();

    // 初始化 GPIO 模块
    GPIO gpio;
    gpio.gpio_init();

    // 注册 PIR 事件处理器（用于日志输出）
    PIREventHandler pirHandler(gpio);
    gpio.registerCallback(PIR_IO, &pirHandler);

    // 创建 I2cDisplayHandle 实例，处理 PIR、DHT 和键盘事件
    I2cDisplayHandle displayHandle;
    gpio.registerCallback(PIR_IO, &displayHandle);

    // 初始化键盘，并注册键盘按键回调
    if (!initKeyboard(gpio)) {
        std::cerr << "⚠️ 键盘初始化失败！" << std::endl;
        return -1;
    }
    std::cout << "🔄 矩阵键盘已启动..." << std::endl;
    // 设置键盘按键回调：当按键被检测时，调用 displayHandle.handleKeyPress()
    KeyboardEventHandler::setKeyPressCallback([&displayHandle](char key) {
        displayHandle.handleKeyPress(key);
    });

    // 启动 GPIO 事件监听线程
    gpio.start();

    // 初始化 DHT11 温湿度传感器，并注册回调，将数据传递给 I2cDisplayHandle 处理
    DHT11 dht11(gpio);
    dht11.registerCallback([&displayHandle](const DHTReading &reading) {
        displayHandle.handleDHT(reading.temp_celsius, reading.humidity);
    });
    dht11.start();

    // 主循环保持运行
    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    cleanupKeyboard();
    gpio.stop();
    std::cout << "退出程序。" << std::endl;
    return 0;
}

