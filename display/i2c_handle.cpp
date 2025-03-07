#include "i2c_handle.h"

I2cDisplayHandler::I2cDisplayHandler(GPIO&gpio) : gpio(gpio) {

}

I2cDisplayHandler::~I2cDisplayHandler() {
    stop();
}

void I2cDisplayHandler::start() {
    // 无需启动线程，直接依赖 GPIO 的事件管理
}

void I2cDisplayHandler::stop() {
    // 无需停止线程
}
void I2cDisplayHandler::handleEvent(const gpiod_line_event& event) {
    if (event.event_type == GPIOD_LINE_EVENT_RISING_EDGE) {
        I2cDisplay::getInstance().displayIntrusion();
        std::cout << "屏幕显示触发报警\n";
    } else if (event.event_type == GPIOD_LINE_EVENT_FALLING_EDGE) {
        I2cDisplay::getInstance().displaySafe();
    }
}