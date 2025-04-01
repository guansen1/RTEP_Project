#ifndef PIR_H
#define PIR_H
#include "gpio/gpio.h"
#include "buzzer/buzzer.h"
#include "keyboard/keyboard.h"
#include "telegram/telegram_listener.h"
class PIREventHandler : public GPIO::GPIOEventCallbackInterface {
    public:
        PIREventHandler(GPIO&gpio, Buzzer&buzzer, TelegramListener&listener);
        ~PIREventHandler();
        void start();
        void stop();
        void handleEvent(const gpiod_line_event& event) override;
    private:
        GPIO& gpio;
        Buzzer&buzzer;
        TelegramListener& listener;
    };

#endif
