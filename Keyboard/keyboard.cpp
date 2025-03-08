#include "keyboard.h"
#include <iostream>

using namespace std;

// **新的 GPIO 引脚定义**
const int rowPins[4] = {1, 7, 8, 12};  // 行（输出）
const int colPins[4] = {16, 23, 24, 25};  // 列（输入，触发事件）

const char keyMap[4][4] = {
    {'1', '2', '3', 'A'},
    {'4', '5', '6', 'B'},
    {'7', '8', '9', 'C'},
    {'*', '0', '#', 'D'}
};

// **初始化键盘**
void initKeyboard(GPIO& gpio) {
    cout << "⌨️ 初始化键盘 GPIO..." << endl;

    // **配置行引脚为输出**
    for (int row : rowPins) {
        gpio.configGPIO(row, OUTPUT);
    }

    // **注册列引脚事件**
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
    int colIndex = -1;
    for (int i = 0; i < 4; i++) {
        if (colPins[i] == event.line_offset) {
            colIndex = i;
            break;
        }
    }

    if (colIndex == -1) return;

    // **检测按下的键**
    for (int row = 0; row < 4; row++) {
        cout << "🔘 按键检测: " << keyMap[row][colIndex] << endl;
    }
}
