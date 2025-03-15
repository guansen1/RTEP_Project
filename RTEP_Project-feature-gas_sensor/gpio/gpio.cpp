#include "gpio.h"

GPIO::GPIO() : chip(nullptr), running(false) {
    chip = gpiod_chip_open_by_name("gpiochip0");
    if (!chip) {
        std::cerr << "无法打开 GPIO 控制器" << std::endl;
        exit(1);
    }
}

GPIO::~GPIO() {
    stop();
    for (auto& pin : gpio_pins) {
        gpiod_line_release(pin.second);
    }
    gpiod_chip_close(chip);
}

void GPIO::gpio_init() {
    configGPIO(PIR_IO, BOTH_EDGES);
    configGPIO(BUZZER_IO, OUTPUT);
    configGPIO(DHT_IO, OUTPUT);
    configGPIO(GS_IO, BOTH_EDGES);
}

bool GPIO::configGPIO(int pin_number, int config_num) {
    
    if (gpio_pins.find(pin_number) != gpio_pins.end()) {
        gpiod_line_release(gpio_pins[pin_number]);
        gpio_pins.erase(pin_number);
        gpio_config.erase(pin_number);
    }

    struct gpiod_line* line = gpiod_chip_get_line(chip, pin_number);
    if (!line) {
        std::cerr << "无法获取 GPIO 引脚 " << pin_number << std::endl;
        return false;
    }

    int request_status = -1;
    switch (config_num) {
        case INPUT: request_status = gpiod_line_request_input(line, "GPIO_input"); break;
        case OUTPUT: request_status = gpiod_line_request_output(line, "GPIO_output", 0); break;
        case INPUT_PULLUP: request_status = gpiod_line_request_input_flags(line, "GPIO_input_pullup", GPIOD_LINE_REQUEST_FLAG_BIAS_PULL_UP); break;
        case INPUT_PULLDOWN: request_status = gpiod_line_request_input_flags(line, "GPIO_input_pulldown", GPIOD_LINE_REQUEST_FLAG_BIAS_PULL_DOWN); break;
        case RISING_EDGE: request_status = gpiod_line_request_both_edges_events(line, "GPIO_edge_rising"); break;
        case FALLING_EDGE: request_status = gpiod_line_request_falling_edge_events(line, "GPIO_edge_falling"); break;
        case BOTH_EDGES: request_status = gpiod_line_request_both_edges_events(line, "GPIO_edge_both"); break;
        default:
            std::cerr << "错误: 未知的 GPIO 配置编号 " << config_num << std::endl;
            gpiod_line_release(line);
            return false;
    }

    if (request_status < 0) {
        std::cerr << "无法配置 GPIO " << pin_number << "（模式: " << config_num << "）\n";
        return false;
    }

    gpio_pins[pin_number] = line;
    gpio_config[pin_number] = config_num;
    return true;
}

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

void GPIO::registerCallback(int pin_number, GPIOEventCallbackInterface* callback) {
    callbacks[pin_number].push_back(callback);
}

void GPIO::start() {
    running = true;
    workerThread = std::thread(&GPIO::worker, this);
}

void GPIO::stop() {
    running = false;
    if (workerThread.joinable()) {
        workerThread.join();
    }
}

void GPIO::worker() {
    while (running) {
        struct timespec timeout = {1, 0}; // 1 秒超时
        for (auto& pin : gpio_pins) {
            if (gpio_config[pin.first] == 4 || gpio_config[pin.first] == 5 || gpio_config[pin.first] == 6) {
                int result = waitForEvent(pin.first, &timeout);
                if (result == 1) {
                    gpiod_line_event event;
                    if (readEvent(pin.first, event)) {
                        if (callbacks.find(pin.first) != callbacks.end()) {
                            for (auto& callback : callbacks[pin.first]) {
                                callback->handleEvent(event);
                            }
                        }
                    }
                }
            }
        }
    }
}

int GPIO::waitForEvent(int pin_number, struct timespec* timeout) {
    if (gpio_pins.find(pin_number) == gpio_pins.end()) {
        std::cerr << "GPIO " << pin_number << " 未初始化！" << std::endl;
        return -1;
    }
    return gpiod_line_event_wait(gpio_pins[pin_number], timeout);
}

bool GPIO::readEvent(int pin_number, gpiod_line_event& event) {
    if (gpio_pins.find(pin_number) == gpio_pins.end()) {
        std::cerr << "GPIO " << pin_number << " 未初始化！" << std::endl;
        return false;
    }
    if (gpiod_line_event_read(gpio_pins[pin_number], &event) < 0) {
        std::cerr << "读取 GPIO " << pin_number << " 事件失败！\n";
        return false;
    }
    return true;
}