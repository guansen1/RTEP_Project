#include "pir.h"

PIREventHandler::PIREventHandler(GPIO&gpio) : gpio(gpio) {

}

PIREventHandler::~PIREventHandler() {
    stop();
}

void PIREventHandler::start() {
    // 无需启动线程，直接依赖 GPIO 的事件管理
}

void PIREventHandler::stop() {
    // 无需停止线程
}

void PIREventHandler::handleEvent(const gpiod_line_event& event) {
    if (event.event_type == GPIOD_LINE_EVENT_RISING_EDGE) {
        std::cout << "[PIR] 运动检测触发！（上升沿）\n";
    } else if (event.event_type == GPIOD_LINE_EVENT_FALLING_EDGE) {
        std::cout << "[PIR] 运动信号消失！（下降沿）\n";
    }
}
