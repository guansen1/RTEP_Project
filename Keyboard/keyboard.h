#ifndef KEYBOARD_H
#define KEYBOARD_H

#include "gpio/gpio.h"
#include <string>

// **初始化键盘**
void initKeyboard(GPIO& gpio);

// **释放键盘 GPIO**
void cleanupKeyboard();

// **键盘事件回调**
class KeyboardEventHandler : public GPIO::GPIOEventCallbackInterface {
public:
    KeyboardEventHandler();
    virtual ~KeyboardEventHandler();
    void handleEvent(const gpiod_line_event& event) override;
private:
    std::string input_buffer; // 存储输入密码
};

#endif
