#ifndef PIR_H
#define PIR_H
#include "gpio/gpio.h"
#include "buzzer/buzzer.h"
class PIREventHandler : public GPIO::GPIOEventCallbackInterface {
    public:
        PIREventHandler(GPIO&gpio, Buzzer&buzzer);
        ~PIREventHandler();
        void start();
        void stop();
        void handleEvent(const gpiod_line_event& event) override;
    private:
        GPIO& gpio;
        Buzzer&buzzer;
    };

#endif
