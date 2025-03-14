#include "Keyboard/keyboard.h"
#include <iostream>
#include <chrono>
#include <thread>

// 矩阵键盘 GPIO 引脚定义（根据你的硬件调整）
const int rowPins[4] = {KB_R1_IO, KB_R2_IO, KB_R3_IO, KB_R4_IO}; // 行引脚
const int colPins[4] = {KB_R5_IO, KB_R6_IO, KB_R7_IO, KB_R8_IO}; // 列引脚

Keyboard::Keyboard(GPIO& gpio) : gpio(gpio) {}

Keyboard::~Keyboard() {
    cleanup();
}

void Keyboard::init() {
    std::cout << "⌨️ 初始化键盘 GPIO..." << std::endl;
    
    // 设置行引脚为输出，初始低电平
    for (int row : rowPins) {
        gpio.setDirection(row, OUTPUT);
        gpio.write(row, LOW);
    }
    
    // 设置列引脚为输入，上拉电阻，并注册下降沿事件
    for (int col : colPins) {
        gpio.setDirection(col, INPUT);
        gpio.setPullUpDown(col, PULL_UP);
        auto* handler = new KeyboardEventHandler(this, col);
        handlers.push_back(handler);
        gpio.registerCallback(col, handler, GPIOD_LINE_EVENT_FALLING_EDGE);
    }
}

void Keyboard::cleanup() {
    std::cout << "🔚 释放键盘 GPIO 资源" << std::endl;
    for (auto* handler : handlers) {
        delete handler;
    }
    handlers.clear();
}

void Keyboard::scanRowsAndProcess(int colIndex) {
    // 去抖：等待20ms确认按键稳定
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
    if (gpio.read(colPins[colIndex]) == HIGH) {
        return; // 按键已释放，退出
    }

    // 扫描所有行
    for (int row = 0; row < 4; row++) {
        gpio.write(rowPins[row], HIGH); // 将当前行置高
        std::this_thread::sleep_for(std::chrono::microseconds(10)); // 短暂延迟
        if (gpio.read(colPins[colIndex]) == LOW) { // 检查列是否仍为低
            char key = keyMap[row][colIndex];
            std::cout << "🔘 检测到按键: " << key << std::endl;
            processKeyPress(row, colIndex);
        }
        gpio.write(rowPins[row], LOW); // 恢复低电平
    }
}

void Keyboard::processKeyPress(int row, int col) {
    std::cout << "🔘 按键: " << keyMap[row][col] << std::endl;
    // 在此添加密码处理逻辑，例如：
    // if (keyMap[row][col] == '#') { /* 验证密码 */ }
}

KeyboardEventHandler::KeyboardEventHandler(Keyboard* parent, int pin) 
    : parent(parent), associatedPin(pin) {}

void KeyboardEventHandler::handleEvent(const gpiod_line_event& event) {
    if (event.event_type == GPIOD_LINE_EVENT_FALLING_EDGE) {
        int colIndex = -1;
        for (int i = 0; i < 4; i++) {
            if (colPins[i] == associatedPin) {
                colIndex = i;
                break;
            }
        }
        if (colIndex != -1) {
            std::cout << "🔍 列 " << colIndex << " 触发" << std::endl;
            parent->scanRowsAndProcess(colIndex);
        }
    }
}



/////main.cpp



#include <iostream>
#include <thread>
#include <chrono>
#include "gpio/gpio.h"
#include "pir/pir.h"
#include "dht/dht.h"
#include "display/i2c_display.h"
#include "i2c_handle.h"
#include "Keyboard/keyboard.h" 

int main() {
    std::cout << "系统启动！" << std::endl;

    // 初始化 I2C 显示模块（SSD1306）
    I2cDisplay::getInstance().init();

    // 初始化 GPIO 模块
    GPIO gpio;
    gpio.gpio_init();

    // 注册 PIR 事件处理器（用于日志输出等）
    PIREventHandler pirHandler(gpio);
    gpio.registerCallback(PIR_IO, &pirHandler);

    // 创建 I2cDisplayHandle 实例，处理 PIR 和 DHT 事件
    I2cDisplayHandle displayHandle;
    gpio.registerCallback(PIR_IO, &displayHandle);

    // 启动 GPIO 事件监听线程
    gpio.start();

    // 初始化 DHT11 温湿度传感器，并注册回调
    DHT11 dht11(gpio);
    dht11.registerCallback([&displayHandle](const DHTReading &reading) {
        displayHandle.handleDHT(reading.temp_celsius, reading.humidity);
    });
    dht11.start();
    
    // 初始化矩阵键盘
    Keyboard keyboard(gpio);
    keyboard.init();
    std::cout << "🔄 矩阵键盘已启动..." << std::endl;
    
    // 主循环保持运行
    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    // 释放资源（因无限循环，通常不会执行到这里）
    keyboard.cleanup();
    gpio.stop();
    std::cout << "退出程序。" << std::endl;
    return 0;
}


////  keyboard.cpp

#include "Keyboard/keyboard.h"
#include <iostream>
#include <thread>
#include <gpiod.h>

// 矩阵键盘 GPIO 引脚定义
const int rowPins[4] = {KB_R1_IO, KB_R2_IO, KB_R3_IO, KB_R4_IO};  // 行（输出）
const int colPins[4] = {KB_R5_IO, KB_R6_IO, KB_R7_IO, KB_R8_IO};  // 列（输入）

Keyboard::Keyboard(GPIO& gpio) : gpio(gpio) {}

Keyboard::~Keyboard() {
    cleanup();
}

void Keyboard::init() {
    std::cout << "⌨️ 初始化键盘 GPIO..." << std::endl;

    // 配置行引脚为输出，初始高电平
    for (int row : rowPins) {
        gpio.configGPIO(row, GPIOconfig::OUTPUT);
        gpio.writeGPIO(row, 1); // 设置为高电平
    }

    // 配置列引脚为输入，启用上拉电阻，并注册下降沿事件
    for (int col : colPins) {
        // 配置为下降沿事件并启用上拉
        gpio.configGPIO(col, GPIOconfig::FALLING_EDGE, GPIOD_LINE_REQUEST_FLAG_BIAS_PULL_UP);
        auto* handler = new KeyboardEventHandler(this, col);
        gpio.registerCallback(col, handler);
        handlers.push_back(handler);
    }
}

void Keyboard::cleanup() {
    std::cout << "🔚 释放键盘 GPIO 资源" << std::endl;
    for (auto* handler : handlers) {
        delete handler;
    }
    handlers.clear();
}

void Keyboard::processKeyPress(int row, int col) {
    std::cout << "🔘 按键: " << keyMap[row][col] << " (Row: " << row << ", Col: " << col << ")" << std::endl;
}

KeyboardEventHandler::KeyboardEventHandler(Keyboard* parent, int pin) : parent(parent), associatedPin(pin) {}

void KeyboardEventHandler::handleEvent(const gpiod_line_event& event) {
    // 去抖: 等待20ms以确保稳定
    std::this_thread::sleep_for(std::chrono::milliseconds(20));

    // 仅处理下降沿（按键按下）
    if (event.event_type != GPIOD_LINE_EVENT_FALLING_EDGE) return;

    std::cout << "🔍 触发 GPIO 事件，pin: " << associatedPin << std::endl;

    // 确认是列引脚
    int colIndex = -1;
    for (int i = 0; i < 4; i++) {
        if (colPins[i] == associatedPin) {
            colIndex = i;
            break;
        }
    }
    if (colIndex == -1) return; // 不是列引脚，忽略

    // 扫描行以找到按下的键
    for (int row = 0; row < 4; row++) {
        // 将当前行设置为低电平（激活）
        parent->getGPIO().writeGPIO(rowPins[row], 0);
        // 短暂延迟以稳定信号
        std::this_thread::sleep_for(std::chrono::microseconds(10));
        // 检查列是否为低电平（按键按下）
        if (parent->getGPIO().readGPIO(colPins[colIndex]) == 0) {
            parent->processKeyPress(row, colIndex);
            // 检测到一个按键后退出扫描（假设单键按下）
            parent->getGPIO().writeGPIO(rowPins[row], 1);
            break;
        }
        // 将行重置为高电平
        parent->getGPIO().writeGPIO(rowPins[row], 1);
    }
}


//////  keyboard,h

#ifndef KEYBOARD_H
#define KEYBOARD_H

#include "gpiod.h"
#include "gpio/gpio.h"
#include <iostream>
#include <chrono>
#include <vector>

// 矩阵键盘 GPIO 引脚定义
extern const int rowPins[4]; // 行（输出）
extern const int colPins[4]; // 列（输入）

// 按键映射表
const char keyMap[4][4] = {
    {'1', '2', '3', 'A'},
    {'4', '5', '6', 'B'},
    {'7', '8', '9', 'C'},
    {'*', '0', '#', 'D'}
};

// 键盘事件处理类
class KeyboardEventHandler : public GPIO::GPIOEventCallbackInterface {
public:
    KeyboardEventHandler(Keyboard* parent, int pin);
    void handleEvent(const gpiod_line_event& event) override;

private:
    Keyboard* parent;
    int associatedPin; // 保存注册时传入的 GPIO 引脚编号
};

// 键盘管理类
class Keyboard {
public:
    explicit Keyboard(GPIO& gpio);
    ~Keyboard();

    void init();
    void cleanup();
    void processKeyPress(int row, int col);
    
    GPIO& getGPIO() { return gpio; }  // 提供访问 GPIO 的公共方法
    
private:
    GPIO& gpio;
    std::vector<KeyboardEventHandler*> handlers;
};

#endif // KEYBOARD_H


/////// gpio.cpp

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
    // 键盘引脚配置已移至 Keyboard 类
}

bool GPIO::configGPIO(int pin_number, int config_num, int flags) {
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

    struct gpiod_line_request_config config = {};
    config.consumer = "GPIO";
    int initial_value = 0; // 输出模式的默认初始值

    switch (config_num) {
        case INPUT:
            config.request_type = GPIOD_LINE_REQUEST_DIRECTION_INPUT;
            break;
        case OUTPUT:
            config.request_type = GPIOD_LINE_REQUEST_DIRECTION_OUTPUT;
            break;
        case INPUT_PULLUP:
            config.request_type = GPIOD_LINE_REQUEST_DIRECTION_INPUT;
            config.flags |= GPIOD_LINE_REQUEST_FLAG_BIAS_PULL_UP;
            break;
        case INPUT_PULLDOWN:
            config.request_type = GPIOD_LINE_REQUEST_DIRECTION_INPUT;
            config.flags |= GPIOD_LINE_REQUEST_FLAG_BIAS_PULL_DOWN;
            break;
        case RISING_EDGE:
            config.request_type = GPIOD_LINE_REQUEST_EVENT_RISING_EDGE;
            break;
        case FALLING_EDGE:
            config.request_type = GPIOD_LINE_REQUEST_EVENT_FALLING_EDGE;
            break;
        case BOTH_EDGES:
            config.request_type = GPIOD_LINE_REQUEST_EVENT_BOTH_EDGES;
            break;
        default:
            std::cerr << "错误: 未知的 GPIO 配置编号 " << config_num << std::endl;
            gpiod_line_release(line);
            return false;
    }

    config.flags |= flags; // 添加用户指定的标志

    if (config.request_type == GPIOD_LINE_REQUEST_DIRECTION_OUTPUT) {
        if (gpiod_line_request(line, &config, initial_value) < 0) {
            std::cerr << "无法配置 GPIO " << pin_number << "（模式: " << config_num << "）\n";
            return false;
        }
    } else {
        if (gpiod_line_request(line, &config, 0) < 0) {
            std::cerr << "无法配置 GPIO " << pin_number << "（模式: " << config_num << "）\n";
            return false;
        }
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
    if (gpio_config[pin_number] != OUTPUT) {
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
        struct timespec timeout = {1, 0}; // 1秒超时
        for (auto& pin : gpio_pins) {
            if (gpio_config[pin.first] == RISING_EDGE || 
                gpio_config[pin.first] == FALLING_EDGE || 
                gpio_config[pin.first] == BOTH_EDGES) {
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


///// gpio.h


#ifndef GPIO_H
#define GPIO_H

#include <gpiod.h>
#include <iostream>
#include <unordered_map>
#include <vector>
#include <thread>
#include <atomic>
#include <memory>

enum GPIOconfig {
    INPUT = 0,              // 输入模式
    OUTPUT = 1,             // 输出模式
    INPUT_PULLUP = 2,       // 上拉输入
    INPUT_PULLDOWN = 3,     // 下拉输入
    RISING_EDGE = 4,        // 上升沿触发事件
    FALLING_EDGE = 5,       // 下降沿触发事件
    BOTH_EDGES = 6          // 双边沿触发事件
};

enum GPIOdef {
    KB_R1_IO = 1,
    KB_R2_IO = 7,
    KB_R3_IO = 8,
    KB_R4_IO = 11,
    KB_R5_IO = 12,
    PIR_IO = 14,
    BUZZER_IO = 15,
    KB_R6_IO = 16,
    DHT_IO = 18,
    KB_R7_IO = 20,
    KB_R8_IO = 21
};

class GPIO {
public:
    struct GPIOEventCallbackInterface {
        virtual void handleEvent(const gpiod_line_event& event) = 0;
        virtual ~GPIOEventCallbackInterface() = default;
    };

    GPIO();
    ~GPIO();

    void gpio_init();
    bool configGPIO(int pin_number, int config_num, int flags = 0);
    int readGPIO(int pin_number);
    bool writeGPIO(int pin_number, int value);
    void registerCallback(int pin_number, GPIOEventCallbackInterface* callback);
    void start();
    void stop();

private:
    void worker();
    int waitForEvent(int pin_number, struct timespec* timeout);
    bool readEvent(int pin_number, gpiod_line_event& event);

    struct gpiod_chip* chip;
    std::unordered_map<int, struct gpiod_line*> gpio_pins;
    std::unordered_map<int, int> gpio_config;
    std::unordered_map<int, std::vector<GPIOEventCallbackInterface*>> callbacks;
    std::thread workerThread;
    std::atomic<bool> running;
};

#endif // GPIO_H

/////


