#ifndef I2C_HANDLE_H
#define I2C_HANDLE_H
#include "gpio/gpio.h"
#include "i2c_display.h"
class I2cDisplayHandler : public GPIO::GPIOEventCallbackInterface {
    public:
        I2cDisplayHandler(GPIO&gpio);
        ~I2cDisplayHandler();
        void start();
        void stop();
        void handleEvent(const gpiod_line_event& event) override;
    private:
        GPIO& gpio;
    };

#endif