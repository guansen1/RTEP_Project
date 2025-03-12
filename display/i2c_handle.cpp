#include "i2c_handle.h"
#include <iostream>

I2cDisplayHandle::I2cDisplayHandle() : state(DisplayState::SAFE), inputBuffer("") {
}

I2cDisplayHandle::~I2cDisplayHandle() {
}

void I2cDisplayHandle::handleEvent(const gpiod_line_event& event) {
    if (event.event_type == GPIOD_LINE_EVENT_RISING_EDGE) {
        state = DisplayState::INVASION;
        // 当进入 INVASION 状态时清空密码输入
        inputBuffer.clear();
        I2cDisplay::getInstance().displayIntrusion();
        std::cout << "[I2cDisplayHandle] PIR上升沿：状态切换为 INVASION" << std::endl;
    }
}

void I2cDisplayHandle::handleDHT(float temp, float humidity) {
    if (state == DisplayState::SAFE) {
        std::string tempStr = "Temp:" + std::to_string(temp) + " C";
        std::string humStr  = "Hum:" + std::to_string(humidity) + " %";
        I2cDisplay::getInstance().displaySafeAndDHT(tempStr, humStr);
        std::cout << "[I2cDisplayHandle] DHT更新：温度 " << temp 
                  << " C, 湿度 " << humidity << "%" << std::endl;
    }
}

void I2cDisplayHandle::handleKeyPress(char key) {
    // 仅在 INVASION 状态下处理键盘输入
    if (state == DisplayState::INVASION) {
        // 只接受数字键输入
        if (key >= '0' && key <= '9') {
            inputBuffer.push_back(key);
            // 构造与输入位数相同的 '*' 字符串
            std::string stars(inputBuffer.size(), '*');
            // 更新屏幕显示密码输入反馈
            I2cDisplay::getInstance().displayPasswordStars(stars);
            std::cout << "[I2cDisplayHandle] Password input: " << inputBuffer << std::endl;
            
            // 当输入达到4位时验证密码
            if (inputBuffer.size() == 4) {
                if (inputBuffer == "1234") {
                    state = DisplayState::SAFE;
                    inputBuffer.clear();
                    I2cDisplay::getInstance().displaySafeAndDHT();
                    std::cout << "[I2cDisplayHandle] Correct password, state switched to SAFE" << std::endl;
                } else {
                    std::cout << "[I2cDisplayHandle] Wrong password, please try again." << std::endl;
                    I2cDisplay::getInstance().displayWrongPassword();
                    std::this_thread::sleep_for(std::chrono::seconds(2));
                    I2cDisplay::getInstance().displayIntrusion();
                    inputBuffer.clear();
                }
            }
        }
    }
}
