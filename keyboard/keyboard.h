#ifndef KEYBOARD_H
#define KEYBOARD_H

#include "gpio/gpio.h"
#include <string>
#include <functional>
#include <thread>
#include <atomic>

class ActiveKeyboardScanner {
public:
    // 构造函数接收GPIO对象引用
    ActiveKeyboardScanner(GPIO &gpio);
    ~ActiveKeyboardScanner();

    // 开始主动扫描
    void start();
    // 停止扫描
    void stop();

    // 设置按键回调函数，当检测到按键时调用该回调并传递对应的字符
    void setKeyCallback(std::function<void(char)> callback);

private:
    // 主扫描循环
    void scanLoop();

    GPIO &gpio;
    std::atomic<bool> scanning;
    std::thread scanThread;
    std::function<void(char)> keyCallback;

    // 键盘行和列引脚，按实际硬件接线修改：
    static const int rowPins[4];  // 例如，连接到GPIO 5,6,7,8
    static const int colPins[4];  // 例如，连接到GPIO 1,2,3,4

    // 键盘按键映射表，行与列对应
    static const char keyMap[4][4];
};

#endif // KEYBOARD_H
