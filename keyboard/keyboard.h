#ifndef KEYBOARD_H
#define KEYBOARD_H

#include "gpio/gpio.h"
#include <string.h>
#include <functional>
#include <thread>
#include <atomic>
#include <iostream>
#include <chrono>
#include <unistd.h>
#include <sys/timerfd.h>
#include "display/i2c_display.h"

class I2cDisplayHandle;
class ActiveKeyboardScanner {
public:
    // 构造函数接收GPIO对象引用
    ActiveKeyboardScanner(GPIO &gpio,I2cDisplayHandle &displayHandle);
    ~ActiveKeyboardScanner();

    // 开始主动扫描
    void start();
    // 停止扫描
    void stop();
    void closescan();
    // 设置按键回调函数，当检测到按键时调用该回调并传递对应的字符
    void setKeyCallback(std::function<void(char)> callback);
    void initkeyboard(I2cDisplayHandle& displayHandle);

private:
    // 主扫描循环
    void scanLoop();
    void timerEvent();  // 新增：定时器事件处理函数
    GPIO &gpio;
    std::thread scanThread;
    std::atomic<bool> scanning;
    int timerfd;  // 新增：定时器文件描述符
    std::function<void(char)> keyCallback;
    I2cDisplayHandle &displayHandle;
    // 键盘行和列引脚，按实际硬件接线修改：
    static const int rowPins[4];  // 例如，连接到GPIO 5,6,7,8
    static const int colPins[4];  // 例如，连接到GPIO 1,2,3,4

    // 键盘按键映射表，行与列对应
    static const char keyMap[4][4];
};

#endif // KEYBOARD_H
