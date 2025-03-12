#ifndef KEYBOARD_H
#define KEYBOARD_H

#include "gpiod.h"
#include "gpio/gpio.h"
#include <iostream>
#include <chrono>
#include <vector>

// **矩阵键盘 GPIO 引脚定义**
extern const int rowPins[4]; // 行（事件触发）
extern const int colPins[4]; // 列（事件触发）

// 按键映射表
const char keyMap[4][4] = {
    {'1', '2', '3', 'A'},
    {'4', '5', '6', 'B'},
    {'7', '8', '9', 'C'},
    {'*', '0', '#', 'D'}
};

// **键盘事件处理类**
class KeyboardEventHandler : public GPIO::GPIOEventCallbackInterface {
public:
    // 增加 pin 参数，用于保存当前回调关联的 GPIO 引脚编号
    KeyboardEventHandler(class Keyboard* parent, int pin);
    void handleEvent(const gpiod_line_event& event) override;

private:
    Keyboard* parent;
    int associatedPin; // 保存注册时传入的 GPIO 引脚编号
};

// **键盘管理类**
class Keyboard {
public:
    explicit Keyboard(GPIO& gpio);
    ~Keyboard();

    void init();
    void cleanup();
    void processKeyPress(int row, int col);
    
    GPIO& getGPIO() { return gpio; }  // ✅ 提供访问 GPIO 的 public 方法
    
private:
    GPIO& gpio;
    std::vector<KeyboardEventHandler*> handlers;
    int activeRow = -1, activeCol = -1;
    bool keyDetected = false;
    std::chrono::steady_clock::time_point lastPressTime;
};

#endif // KEYBOARD_H
