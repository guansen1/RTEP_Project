#include "gpio.h"
#include <gpiod.h>
#include <iostream>
#include <unordered_map>

GPIO::GPIO() {
    // 打开 GPIO 控制器（默认使用 gpiochip0）
    chip = gpiod_chip_open_by_name("gpiochip0");
    if (!chip) {
        std::cerr << "无法打开 GPIO 控制器" << std::endl;
        exit(1);
    }
}

GPIO::~GPIO() {
    // 释放所有 GPIO 引脚和关闭控制器
    for (auto& pin : gpio_pins) {
        gpiod_line_release(pin.second);  // 释放每个 GPIO 引脚
    }
    gpiod_chip_close(chip);  // 关闭 GPIO 控制器
}

void GPIO::gpio_init() {
    // 配置 GPIO 14 为输入（PIR 传感器）
    configGPIO(PIR_IO, INPUT);
    configGPIO(BUZZER_IO,OUTPUT);
    configGPIO(TH_IO, INPUT);
}

// 添加 GPIO 引脚并配置为输入
bool GPIO::configGPIO(int pin_number, int config_num) {
    // 获取指定的 GPIO 引脚
    struct gpiod_line* line = gpiod_chip_get_line(chip, pin_number);
    if (!line) {
        std::cerr << "无法获取 GPIO 引脚 " << pin_number << std::endl;
        return false;
    }

    int request_status = -1; // 用于存储 `gpiod_line_request_*` 的返回值

    switch (config_num) {
        case 0: // 输入模式
            request_status = gpiod_line_request_input(line, "GPIO_input");
            break;

        case 1: // 输出模式（默认低电平）
            request_status = gpiod_line_request_output(line, "GPIO_output", 0);
            break;

        case 2: // 上拉输入
            request_status = gpiod_line_request_input_flags(line, "GPIO_input_pullup", GPIOD_LINE_REQUEST_FLAG_BIAS_PULL_UP);
            break;

        case 3: // 下拉输入
            request_status = gpiod_line_request_input_flags(line, "GPIO_input_pulldown", GPIOD_LINE_REQUEST_FLAG_BIAS_PULL_DOWN);
            break;

        case 4: // 上升沿触发
            request_status = gpiod_line_request_both_edges_events(line, "GPIO_edge_rising");
            break;

        case 5: // 下降沿触发
            request_status = gpiod_line_request_falling_edge_events(line, "GPIO_edge_falling");
            break;

        case 6: // 双边沿触发
            request_status = gpiod_line_request_both_edges_events(line, "GPIO_edge_both");
            break;

        default:
            std::cerr << "错误: 未知的 GPIO 配置编号 " << config_num << std::endl;
            gpiod_line_release(line);
            return false;
    }

    // 检查配置是否成功
    if (request_status < 0) {
        std::cerr << "无法配置 GPIO " << pin_number << "（模式: " << config_num << "）\n";
        return false;
    }

    // 将 GPIO 引脚存储到 `gpio_pins` 容器中
    gpio_pins[pin_number] = line;
    gpio_config[pin_number] = config_num;  // 存储 GPIO 配置
    return true;
}


// 读取指定 GPIO 引脚的值
int GPIO::readGPIO(int pin_number) {
    if (gpio_pins.find(pin_number) == gpio_pins.end()) {
        std::cerr << "GPIO 引脚 " << pin_number << " 未初始化！" << std::endl;
        return -1;
    }
    return gpiod_line_get_value(gpio_pins[pin_number]);
}

bool GPIO::writeGPIO(int pin_number, int value) {
    if (gpio_pins.find(pin_number) == gpio_pins.end()) {
        std::cerr << "GPIO " << pin_number << " 未初始化！" << std::endl;
        return false;
    }
    if (gpio_config[pin_number] != 1) {
        std::cerr << "GPIO " << pin_number << " 不是输出模式，无法写入！" << std::endl;
        return false;
    }
    gpiod_line_set_value(gpio_pins[pin_number], value);
    return true;
}

struct gpiod_chip* chip;  // GPIO 控制器
std::unordered_map<int, struct gpiod_line*> gpio_pins;  // 存储所有 GPIO 引脚
std::unordered_map<int, int> gpio_config;  // 存储 GPIO 配置（0=输入, 1=输出）



