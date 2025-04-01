#ifndef BUZZER_H
#define BUZZER_H

#include "gpio/gpio.h"
#include <iostream>
#include <fstream>
#include <unistd.h>
#include "rpi_pwm.h"

#define BUZZER_PWM_CHANNEL 2
class Buzzer {
    public:
        Buzzer(RPI_PWM pwm);
        ~Buzzer();
        void enable(int freq);
        void disable();
    private:
        RPI_PWM pwm;
};

#endif // BUZZER_H
