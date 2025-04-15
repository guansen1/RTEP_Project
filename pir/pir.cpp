#include "pir.h"
#include "telegram/telegram_listener.h"
#include "telegram/telegram.h"
PIREventHandler::PIREventHandler(GPIO&gpio, Buzzer& buzzer, TelegramListener& listener) 
    : gpio(gpio), buzzer(buzzer), listener(listener) {}

PIREventHandler::~PIREventHandler() {
    stop();
}

void PIREventHandler::start() {
    // no need to start thread，rely on GPIO event management directly
}

void PIREventHandler::stop() {
    // no need to stop thread
}

void PIREventHandler::handleEvent(const gpiod_line_event& event) {
    if (event.event_type == GPIOD_LINE_EVENT_RISING_EDGE) {
        std::cout << "[PIR] movement detected！（rising edge）\n";
        
        std::string token = "7415933593:AAH3hw9NeuMsAOeuqyUZe_l935KP2mxaYGA";
        std::string chatId = "7262701565";        
        std::string message = "warning：movement detected！";
        
        //if (listener.isAlarmEnabled()) {
            buzzer.enable(1000);
        //} else {
            //std::cout << "[PIR] alarm function has been disabled, The buzzer stops working。\n";
        //
        // call sendTelegramMessage 
        if (sendTelegramMessage(token, chatId, message)) {
            std::cout << "Telegram message sends successfully" << std::endl;
        } else {
            std::cerr << "Telegram message sends failed" << std::endl;
        }
    } 
    
}
