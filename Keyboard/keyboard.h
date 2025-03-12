#ifndef KEYBOARD_H
#define KEYBOARD_H

#include "gpio/gpio.h"
#include <iostream>
#include <chrono>
#include <vector>

// **矩阵键盘 GPIO 引脚定义**
const int rowPins[4] = {1, 7, 8, 11};  // 行（事件触发）
const int colPins[4] = {12, 16, 20, 21};  // 列（事件触发）

const char keyMap[4][4] = {
    {'1', '2', '3', 'A'},
    {'4', '5', '6', 'B'},
    {'7', '8', '9', 'C'},
    {'*', '0', '#', 'D'}
};

// **键盘事件处理类**
class KeyboardEventHandler : public GPIO::GPIOEventCallbackInterface {
public:
    explicit KeyboardEventHandler(class Keyboard* parent);
    virtual ~KeyboardEventHandler() = default;
    void handleEvent(const gpiod_line_event& event) override;

private:
    Keyboard* parent;
};

// **键盘管理类**
class Keyboard {
public:
    explicit Keyboard(GPIO& gpio);
    ~Keyboard();

    void init();
    void cleanup();
    void processKeyPress(int row, int col);

private:
    GPIO& gpio;
    std::vector<KeyboardEventHandler*> handlers;
    int activeRow = -1, activeCol = -1;
    bool keyDetected = false;
    std::chrono::steady_clock::time_point lastPressTime;
};

#endif // KEYBOARD_H
