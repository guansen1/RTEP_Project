#ifndef GS_H
#define GS_H
#include "gpio/gpio.h"

class GSEventHandler : public GPIO::GPIOEventCallbackInterface {
    public:
        GSEventHandler(GPIO&gpio);
        ~GSEventHandler();
        void start();
        void stop();
        void handleEvent(const gpiod_line_event& event) override;
    private:
        GPIO& gpio;
    };

#endif