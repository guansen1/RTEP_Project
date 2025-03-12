#include "Keyboard/keyboard.h"
#include "i2c_display.h"
#include <iostream>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>
#include <cstring>
#include <cerrno>
#include <cstdlib>
#include <chrono>

// **矩阵键盘 GPIO 引脚定义**
//const int rowPins[4] = {1, 7, 8, 11};  // 行（事件触发）
//const int colPins[4] = {12, 16, 20, 21};  // 列（事件触发）

    const int rowPins[4] = {KB_R1_IO, KB_R2_IO, KB_R3_IO, KB_R4_IO};  // 行（事件触发）
    const int colPins[4] = {KB_R5_IO, KB_R6_IO, KB_R7_IO, KB_R8_IO};  // 列（事件触发）
    

using namespace std;

Keyboard::Keyboard(GPIO& gpio) : gpio(gpio) {}

Keyboard::~Keyboard() {
    cleanup();
}

void Keyboard::init() {
    cout << "⌨️ 初始化键盘 GPIO..." << endl;
    for (int row : rowPins) {
        auto* handler = new KeyboardEventHandler(this);
        gpio.registerCallback(row, handler);
        handlers.push_back(handler);
    }
    for (int col : colPins) {
        auto* handler = new KeyboardEventHandler(this);
        gpio.registerCallback(col, handler);
        handlers.push_back(handler);
    }
}

void Keyboard::cleanup() {
    cout << "🔚 释放键盘 GPIO 资源" << endl;
    for (auto* handler : handlers) {
        delete handler;
    }
    handlers.clear();
}

void Keyboard::processKeyPress(int row, int col) {
    cout << "🔘 按键: " << keyMap[row][col] << endl;
}

KeyboardEventHandler::KeyboardEventHandler(Keyboard* parent) : parent(parent) {}

void KeyboardEventHandler::handleEvent(const gpiod_line_event& event) {
    static bool keyDetected = false;
    static auto lastPressTime = chrono::steady_clock::now();

    int pin_number = event.line;          /////////////////////////////////
    int rowIndex = -1, colIndex = -1;
    
    // 检测行
    for (int i = 0; i < 4; i++) {
        if (rowPins[i] == pin_number) {
            rowIndex = i;
            break;
        }
    }

    // 检测列
    for (int i = 0; i < 4; i++) {
        if (colPins[i] == pin_number) {    
            colIndex = i;
            break;
        }
    }

    // 行列触发后确认按键
    if (rowIndex != -1 && colIndex != -1 && !keyDetected) {
        auto now = chrono::steady_clock::now();
        if (chrono::duration_cast<chrono::milliseconds>(now - lastPressTime).count() > 50) { // 去抖
            parent->processKeyPress(rowIndex, colIndex);
            keyDetected = true;
            lastPressTime = now;
        }
    }

    // 按键松开时重置状态
    if (event.event_type == GPIOD_LINE_EVENT_FALLING_EDGE) {
        keyDetected = false;
    }
}
