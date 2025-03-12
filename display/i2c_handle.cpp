#include "i2c_handle.h"
#include <iostream>
#include <thread>
#include <string>

I2cDisplayHandle::I2cDisplayHandle() 
    : state(DisplayState::SAFE), inputBuffer(""), lastTemp(0.0f), lastHumidity(0.0f) {
}

I2cDisplayHandle::~I2cDisplayHandle() {
}

void I2cDisplayHandle::handleEvent(const gpiod_line_event& event) {
    if (event.event_type == GPIOD_LINE_EVENT_RISING_EDGE) {
        state = DisplayState::INVASION;
        inputBuffer.clear();  // 进入入侵状态时清空密码输入
        I2cDisplay::getInstance().displayIntrusion();
        std::cout << "[I2cDisplayHandle] PIR rising: state switched to INVASION" << std::endl;
    }
    // 此处不对下降沿做处理，保持INVASION状态直到键盘解除
}

void I2cDisplayHandle::handleDHT(float temp, float humidity) {
    // 更新最新温湿度数据
    lastTemp = temp;
    lastHumidity = humidity;
    if (state == DisplayState::SAFE) {
        std::string tempStr = "Temp:" + std::to_string(temp) + " C";
        std::string humStr  = "Hum:"  + std::to_string(humidity) + " %";
        I2cDisplay::getInstance().displaySafeAndDHT(tempStr, humStr);
        std::cout << "[I2cDisplayHandle] DHT update: Temp " << temp 
                  << " C, Hum " << humidity << "%" << std::endl;
    }
}

void I2cDisplayHandle::handleKeyPress(char key) {
    // 仅在 INVASION 状态下处理键盘输入
    if (state == DisplayState::INVASION) {
        // 只接受数字键输入
        if (key >= '0' && key <= '9') {
            inputBuffer.push_back(key);
            std::cout << "[I2cDisplayHandle] Password input: " << inputBuffer << std::endl;
            // 当输入达到4位时验证密码
            if (inputBuffer.size() == 4) {
                if (inputBuffer == "1234") {
                    state = DisplayState::SAFE;
                    inputBuffer.clear();
                    // 使用最新的 DHT 数据更新显示 SAFE 状态及温湿度数据
                    std::string tempStr = "Temp:" + std::to_string(lastTemp) + " C";
                    std::string humStr  = "Hum:"  + std::to_string(lastHumidity) + " %";
                    I2cDisplay::getInstance().displaySafeAndDHT(tempStr, humStr);
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
