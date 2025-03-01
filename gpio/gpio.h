#ifndef GPIO_H
#define GPIO_H

#include <gpiod.h>
#include <iostream>
#include <unordered_map>

#define INPUT 0
#define OUTPUT 1

#define PIR_IO 14
#define BUZZER_IO 15
#define TH_IO 18

class GPIO{
    public:
        GPIO();  // 构造函数
        ~GPIO(); // 析构函数
        void gpio_init();
        bool configGPIO(int pin_number, int config_num);
        int readGPIO(int pin_number);
        bool writeGPIO(int pin_number, int value);
    private:
        struct gpiod_chip* chip;  // GPIO 控制器
        std::unordered_map<int, struct gpiod_line*> gpio_pins;  // 存储所有 GPIO 引脚
        std::unordered_map<int, int> gpio_config;  // 存储 GPIO 配置（0=输入, 1=输出）
};

#endif