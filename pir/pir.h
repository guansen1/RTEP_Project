#ifndef PIR_H
#define PIR_H
#include "gpio/gpio.h"

class PIREventHandler : public GPIO::GPIOEventCallbackInterface {
    public:
        PIREventHandler(GPIO&gpio);
        ~PIREventHandler();
        void start();
        void stop();
        void handleEvent(const gpiod_line_event& event) override;
    private:
        GPIO& gpio;
    };

#endif