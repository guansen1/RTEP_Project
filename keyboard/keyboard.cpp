#include "keyboard.h"
#include <iostream>
#include <chrono>
#include <thread>

using namespace std;

// 根据实际硬件接线修改以下行和列对应的GPIO编号
// 此处假设数据手册中：
// COL 1-4 分别接到 GPIO 1,2,3,4；
// ROW 1-4 分别接到 GPIO 5,6,7,8。
// keyboard.cpp 中添加如下定义（加在文件顶部，紧接着 include 部分）
const int ActiveKeyboardScanner::rowPins[4] = {13, 16, 20, 21};
const int ActiveKeyboardScanner::colPins[4] = {1, 7, 8, 26};


// 4×4 键盘按键映射表（示例映射，按数据手册或实际键盘修改）
const char ActiveKeyboardScanner::keyMap[4][4] = {
    {'1', '2', '3', 'A'},
    {'4', '5', '6', 'B'},
    {'7', '8', '9', 'C'},
    {'*', '0', '#', 'D'}
};

ActiveKeyboardScanner::ActiveKeyboardScanner(GPIO &gpioRef)
    : gpio(gpioRef), scanning(false)
{
    // 配置行引脚为输入并启用内部上拉
    for (int i = 0; i < 4; i++) {
        if (!gpio.configGPIO(rowPins[i],FALLING_EDGE )) {
            std::cerr << "Failed to configure row pBOTH_EDGESin " << rowPins[i] << std::endl;
        }
    }
    // 配置列引脚为输出
    for (int i = 0; i < 4; i++) {
        if (!gpio.configGPIO(colPins[i], RISING_EDGE)) {
            std::cerr << "Failed to configure col pin " << colPins[i] << std::endl;
        }
        // 初始设置列为高电平
        gpio.writeGPIO(colPins[i], 1);
    }
}

ActiveKeyboardScanner::~ActiveKeyboardScanner() {
    stop();
}

void ActiveKeyboardScanner::setKeyCallback(std::function<void(char)> callback) {
    keyCallback = callback;
}

void ActiveKeyboardScanner::start() {
    scanning = true;
    scanThread = std::thread(&ActiveKeyboardScanner::scanLoop, this);
}

void ActiveKeyboardScanner::stop() {
    scanning = false;
    if (scanThread.joinable())
        scanThread.join();
}

void ActiveKeyboardScanner::scanLoop() {
    // 主扫描循环：依次将每个列驱动为低电平，再读取所有行状态
    while (scanning) {
        for (int col = 0; col < 4; col++) {
            // 将当前列设置为低电平
            gpio.writeGPIO(colPins[col], 0);
            // 等待电平稳定（10毫秒）
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            // 检查所有行
            for (int row = 0; row < 4; row++) {
                int value = gpio.readGPIO(rowPins[row]);
                // 当按键按下时，由于按键将行与低电平的列短接，行电平会被拉低
                if (value == 0) {
                    char key = keyMap[row][col];
                    if (keyCallback) {
                        keyCallback(key);
                    } else {
                        std::cout << "[ActiveKeyboard] Key pressed: " << key << std::endl;
                    }
                    // 等待按键释放（简化处理），避免连续检测
                    while (gpio.readGPIO(rowPins[row]) == 0 && scanning) {
                        std::this_thread::sleep_for(std::chrono::milliseconds(20));
                    }
                }
            }
            // 将当前列恢复为高电平
            gpio.writeGPIO(colPins[col], 1);
            // 列与列之间等待一段时间
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }
}
