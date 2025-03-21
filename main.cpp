#include <iostream>
#include <thread>
#include <chrono>
#include "gpio/gpio.h"
#include "keyboard.h"

int main() {
    std::cout << "Active Keyboard Test Starting..." << std::endl;

    // 初始化 GPIO 模块
    GPIO gpio;
    gpio.gpio_init();

    // 初始化主动扫描键盘模块
    ActiveKeyboardScanner scanner(gpio);
    scanner.setKeyCallback([](char key) {
        std::cout << "[Callback] Key pressed: " << key << std::endl;
    });

    scanner.start();

    // 主循环保持运行
    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    gpio.stop();
    return 0;
}
