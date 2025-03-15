#include "pir.h"
#include "telegram.h"  // 添加 telegram 模块头文件

PIREventHandler::PIREventHandler(GPIO &gpio) : gpio(gpio) { }

PIREventHandler::~PIREventHandler() {
    stop();
}

void PIREventHandler::start() {
    // 无需启动线程，直接依赖 GPIO 的事件管理
}

void PIREventHandler::stop() {
    // 无需停止线程
}

void PIREventHandler::handleEvent(const gpiod_line_event &event) {
    if (event.event_type == GPIOD_LINE_EVENT_RISING_EDGE) {
        std::cout << "[PIR] 运动检测触发！（上升沿）\n";
        
        // 当检测到运动时，通过 Telegram 发送报警消息
        std::string token = "7415933593:AAH3hw9NeuMsAOeuqyUZe_l935KP2mxaYGA";
        std::string chat_id = "7262701565";     
        std::string message = "警报：检测到运动！";
        
        if (sendTelegramMessage(token, chat_id, message)) {
            std::cout << "[PIR] Telegram 消息发送成功！" << std::endl;
        } else {
            std::cerr << "[PIR] Telegram 消息发送失败！" << std::endl;
        }
    } else if (event.event_type == GPIOD_LINE_EVENT_FALLING_EDGE) {
        std::cout << "[PIR] 运动信号消失！（下降沿）\n";
    }
}

