头文件 (keyboard.h)：#ifndef KEYBOARD_H
#define KEYBOARD_H

#include "gpiod.h"
#include "gpio/gpio.h"
#include <iostream>
#include <chrono>
#include <vector>

// 矩阵键盘 GPIO 引脚定义（外部定义）
extern const int rowPins[4]; // 行引脚，设置为输出
extern const int colPins[4]; // 列引脚，设置为输入并监听事件

// 按键映射表
const char keyMap[4][4] = {
    {'1', '2', '3', 'A'},
    {'4', '5', '6', 'B'},
    {'7', '8', '9', 'C'},
    {'*', '0', '#', 'D'}
};

// 键盘事件处理类
class KeyboardEventHandler : public GPIO::GPIOEventCallbackInterface {
public:
    KeyboardEventHandler(class Keyboard* parent, int pin);
    void handleEvent(const gpiod_line_event& event) override;

private:
    Keyboard* parent;
    int associatedPin; // 保存关联的列引脚编号
};

// 键盘管理类
class Keyboard {
public:
    explicit Keyboard(GPIO& gpio);
    ~Keyboard();

    void init();
    void cleanup();
    void processKeyPress(int row, int col);
    void scanRowsAndProcess(int colIndex); // 新增扫描函数
    
    GPIO& getGPIO() { return gpio; }
    
private:
    GPIO& gpio;
    std::vector<KeyboardEventHandler*> handlers;
};

#endif // KEYBOARD_H
