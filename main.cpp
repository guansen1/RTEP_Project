#include "gpio.h"
#include <iostream>
#include <wiringPi.h>

#define PIR_PIN 0      // PIR 传感器对应的引脚（wiringPi 编号）
#define BUZZER_PIN 1   // 蜂鸣器对应的引脚（wiringPi 编号）

int main() {
    // 初始化 GPIO
    if (!Gpio::init()) {
        return 1;
    }

    // 设置引脚模式
    Gpio::setInput(PIR_PIN);
    Gpio::setOutput(BUZZER_PIN);

    std::cout << "等待 PIR 传感器触发..." << std::endl;

    while (true) {
        // 读取 PIR 传感器状态
        if (Gpio::read(PIR_PIN) == HIGH) {
            std::cout << "检测到运动，启动蜂鸣器..." << std::endl;
            Gpio::write(BUZZER_PIN, HIGH);
            delay(1000); // 蜂鸣器响1秒
            Gpio::write(BUZZER_PIN, LOW);
        }
        delay(100); // 每100毫秒检测一次
    }

    return 0;
}

