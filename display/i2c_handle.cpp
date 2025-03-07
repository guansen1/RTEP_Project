#include "i2c_handle.h"
#include <iostream>

I2cDisplayHandle::I2cDisplayHandle() : state(DisplayState::SAFE) {
}

I2cDisplayHandle::~I2cDisplayHandle() {
}

void I2cDisplayHandle::handleEvent(const gpiod_line_event& event) {
    if (event.event_type == GPIOD_LINE_EVENT_RISING_EDGE) {
        state = DisplayState::INVASION;
        I2cDisplay::getInstance().displayIntrusion();
        std::cout << "[I2cDisplayHandle] PIR上升沿：状态切换为 INVASION" << std::endl;
    } else if (event.event_type == GPIOD_LINE_EVENT_FALLING_EDGE) {
        state = DisplayState::SAFE;
        // 下降沿后不直接刷新 SAFE 状态显示，由 DHT 回调统一更新
        std::cout << "[I2cDisplayHandle] PIR下降沿：状态切换为 SAFE" << std::endl;
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
