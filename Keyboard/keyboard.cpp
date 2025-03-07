#include "keyboard.h"
#include <iostream>

using namespace std;

// **定义矩阵键盘 GPIO**
const int rowPins[4] = {4, 17, 27, 22};  // 行（输出）
const int colPins[4] = {5, 6, 13, 19};  // 列（输入，触发事件）

const char keyMap[4][4] = {
    {'1', '2', '3', 'A'},
    {'4', '5', '6', 'B'},
    {'7', '8', '9', 'C'},
    {'*', '0', '#', 'D'}
};

// **初始化键盘**
void initKeyboard(GPIO& gpio) {
    cout << "⌨️ 初始化键盘 GPIO..." << endl;
    for (int col : colPins) {
        gpio.registerCallback(col, new KeyboardEventHandler()); // 🔴 **注册事件**
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
    // **遍历行，检查哪个按键被按下**
    for (int row = 0; row < 4; row++) {
        cout << "🔘 按键检测: " << keyMap[row][event.line_offset] << endl;
    }
}