#include "keyboard.h"
#include <iostream>
#include <chrono>

using namespace std;

// **矩阵键盘 GPIO 引脚定义**
const int rowPins[4] = {1, 7, 8, 11};  // 行（事件触发）
const int colPins[4] = {12, 16, 20, 21};  // 列（事件触发）

const char keyMap[4][4] = {
    {'1', '2', '3', 'A'},
    {'4', '5', '6', 'B'},
    {'7', '8', '9', 'C'},
    {'*', '0', '#', 'D'}
};

// **初始化键盘**
void initKeyboard(GPIO& gpio) {
    cout << "⌨️ 初始化键盘 GPIO..." << endl;

    // **注册行列引脚事件**
    for (int row : rowPins) {
        gpio.registerCallback(row, new KeyboardEventHandler());
    }
    for (int col : colPins) {
        gpio.registerCallback(col, new KeyboardEventHandler());
    }
}

// **释放资源**
void cleanupKeyboard() {
    cout << "🔚 释放键盘 GPIO 资源" << endl;
}

// **键盘事件处理类**
KeyboardEventHandler::KeyboardEventHandler() : input_buffer("") {}

KeyboardEventHandler::~KeyboardEventHandler() {}

void KeyboardEventHandler::handleEvent(const gpiod_line_event& event) {
    static int activeRow = -1, activeCol = -1;
    static bool keyDetected = false;
    static auto lastPressTime = chrono::steady_clock::now();

    int pin = event.line_offset;
    bool isRow = false, isCol = false;

    // **检测是否是行事件**
    for (int i = 0; i < 4; i++) {
        if (rowPins[i] == pin) {
            activeRow = i;
            isRow = true;
            break;
        }
    }

    // **检测是否是列事件**
    for (int i = 0; i < 4; i++) {
        if (colPins[i] == pin) {
            activeCol = i;
            isCol = true;
            break;
        }
    }

    // **行列都触发后再确认按键**
    if (activeRow != -1 && activeCol != -1 && !keyDetected) {
        auto now = chrono::steady_clock::now();
        if (chrono::duration_cast<chrono::milliseconds>(now - lastPressTime).count() > 50) { // 去抖
            cout << "🔘 按键: " << keyMap[activeRow][activeCol] << endl;
            keyDetected = true; // 标记已经识别，避免重复输出
            lastPressTime = now;
        }
    }

    // **按键松开时重置状态**
    if (event.event_type == GPIOD_LINE_EVENT_FALLING_EDGE) {
        keyDetected = false;
        activeRow = -1;
        activeCol = -1;
    }
}
