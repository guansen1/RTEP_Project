#include "Keyboard/keyboard.h"
#include "i2c_display.h"
#include <iostream>
#include <unistd.h>
#include <chrono>

const int rowPins[4] = {KB_R1_IO, KB_R2_IO, KB_R3_IO, KB_R4_IO};  
const int colPins[4] = {KB_R5_IO, KB_R6_IO, KB_R7_IO, KB_R8_IO};  

using namespace std;

Keyboard::Keyboard(GPIO& gpio) : gpio(gpio) {}

Keyboard::~Keyboard() {
    cleanup();
}

void Keyboard::init() {
    cout << "⌨️ 初始化键盘 GPIO..." << endl;

    // ✅ 1. 设置行引脚为输出，高电平
    for (int row : rowPins) {
        gpio.configGPIO(row, OUTPUT);
        gpio.writeGPIO(row, 1);
    }

    // ✅ 2. 设置列引脚为输入，并启用上拉，同时注册中断
    for (int col : colPins) {
        gpio.configGPIO(col, INPUT_PULLUP);
        gpio.registerCallback(col, new KeyboardEventHandler(this, col));
       
    }
}

void Keyboard::cleanup() {
    cout << "🔚 释放键盘 GPIO 资源" << endl;
}

void Keyboard::processKeyPress(int row, int col) {
    cout << "🔘 按键: " << keyMap[row][col] << endl;
}

KeyboardEventHandler::KeyboardEventHandler(Keyboard* parent, int pin)
    : parent(parent), associatedPin(pin) {}

void KeyboardEventHandler::handleEvent(const gpiod_line_event& event) {
    static auto lastPressTime = chrono::steady_clock::now();

    int pin = associatedPin;
    cout << "🔍 触发 GPIO 事件，pin: " << pin << endl;

    int colIndex = -1;
    for (int i = 0; i < 4; i++) {
        if (colPins[i] == pin) {
            colIndex = i;
            break;
        }
    }
    if (colIndex == -1) return;

    int rowIndex = -1;
    for (int i = 0; i < 4; i++) {
        parent->getGPIO().writeGPIO(rowPins[i], 0);  // ✅ 通过 getGPIO() 访问 gpio
      ////  usleep(10000);

        if (parent->getGPIO().readGPIO(pin) == 0) {  // ✅ 通过 getGPIO() 访问 gpio
            rowIndex = i;
        }

        parent->getGPIO().writeGPIO(rowPins[i], 1);  // ✅ 通过 getGPIO() 访问 gpio
    }

    auto now = chrono::steady_clock::now();
    if (rowIndex != -1 && colIndex != -1 &&
        chrono::duration_cast<chrono::milliseconds>(now - lastPressTime).count() > 50) {
        
        cout << "✅ 按键解析成功: " << keyMap[rowIndex][colIndex] << endl;
        parent->processKeyPress(rowIndex, colIndex);
        lastPressTime = now;
    }

    if (event.event_type == GPIOD_LINE_EVENT_FALLING_EDGE) {
        cout << "✅ 按键释放: " << keyMap[rowIndex][colIndex] << endl;
    }
}

