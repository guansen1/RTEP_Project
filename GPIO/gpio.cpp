#include "gpio.h"
#include <iostream>

bool Gpio::init() {
    if (wiringPiSetup() == -1) {
        std::cerr << "wiringPi 初始化失败！" << std::endl;
        return false;
    }
    return true;
}

void Gpio::setInput(int pin) {
    pinMode(pin, INPUT);
}

void Gpio::setOutput(int pin) {
    pinMode(pin, OUTPUT);
}

void Gpio::write(int pin, int value) {
    digitalWrite(pin, value);
}

int Gpio::read(int pin) {
    return digitalRead(pin);
}
