#include "pir.h"
#include "telegram/telegram_listener.h"
#include "telegram/telegram.h"
PIREventHandler::PIREventHandler(GPIO&gpio, Buzzer& buzzer, TelegramListener& listener) 
    : gpio(gpio), buzzer(buzzer), listener(listener) {}

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
        
        std::string token = "7415933593:AAH3hw9NeuMsAOeuqyUZe_l935KP2mxaYGA";
        std::string chatId = "7262701565";        // 替换为你的 ChatID
        std::string message = "警报：检测到运动！";
        
        //if (listener.isAlarmEnabled()) {
            buzzer.enable(1000);
        //} else {
            //std::cout << "[PIR] 报警功能已禁用，蜂鸣器不会响。\n";
        //
        // 调用 sendTelegramMessage 发送消息
        if (sendTelegramMessage(token, chatId, message)) {
            std::cout << "Telegram 消息发送成功" << std::endl;
        } else {
            std::cerr << "Telegram 消息发送失败" << std::endl;
        }
    } 
    
}
