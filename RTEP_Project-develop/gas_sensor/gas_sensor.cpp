#include "gas_sensor.h"

GSEventHandler::GSEventHandler(GPIO&gpio) : gpio(gpio) {

}

GSEventHandler::~GSEventHandler() {
    stop();
}

void GSEventHandler::start() {
    // 无需启动线程，直接依赖 GPIO 的事件管理
}

void GSEventHandler::stop() {
    // 无需停止线程
}

void GSEventHandler::handleEvent(const gpiod_line_event& event) {
    if (event.event_type == GPIOD_LINE_EVENT_RISING_EDGE) {
        std::cout << "检测到烟雾！（上升沿）\n";
    } else if (event.event_type == GPIOD_LINE_EVENT_FALLING_EDGE) {
        std::cout << "烟雾消失！（下降沿）\n";
    }
}
