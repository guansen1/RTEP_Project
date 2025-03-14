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



#include "Keyboard/keyboard.h"
#include <iostream>
#include <thread>
#include <gpiod.h>

// 矩阵键盘 GPIO 引脚定义
const int rowPins[4] = {KB_R1_IO, KB_R2_IO, KB_R3_IO, KB_R4_IO}; // 行（输出）
const int colPins[4] = {KB_R5_IO, KB_R6_IO, KB_R7_IO, KB_R8_IO}; // 列（输入）

using namespace std;

Keyboard::Keyboard(GPIO& gpio) : gpio(gpio), keyDetected(false) {
    lastPressTime = chrono::steady_clock::now();
}

Keyboard::~Keyboard() {
    cleanup();
}

void Keyboard::init() {
    cout << "⌨️ 初始化键盘 GPIO..." << endl;

    // 配置行引脚为输出，初始低电平
    for (int row : rowPins) {
        cout << "配置行引脚 " << row << " 为输出（低电平）" << endl;
        if (!gpio.configGPIO(row, OUTPUT)) {
            cerr << "❌ 行引脚 " << row << " 配置失败！" << endl;
            continue;
        }
        gpio.writeGPIO(row, 0);
        cout << "✅ 行引脚 " << row << " 配置成功" << endl;
    }

    // 配置列引脚为输入，启用上拉电阻，并注册下降沿事件
    for (int col : colPins) {
        cout << "配置列引脚 " << col << " 为下降沿触发" << endl;
        if (!gpio.configGPIO(col, FALLING_EDGE, GPIOD_LINE_REQUEST_FLAG_BIAS_PULL_UP)) {
            cerr << "❌ 列引脚 " << col << " 配置为下降沿触发失败！" << endl;
            continue;
        }
        auto* handler = new KeyboardEventHandler(this, col);
        gpio.registerCallback(col, handler);
        handlers.push_back(handler);
        cout << "✅ 列引脚 " << col << " 配置成功" << endl;
    }

    cout << "✅ 键盘初始化完成" << endl;
}

void Keyboard::cleanup() {
    cout << "🔚 释放键盘 GPIO 资源" << endl;
    for (auto* handler : handlers) {
        delete handler;
    }
    handlers.clear();
}

void Keyboard::processKeyPress(int row, int col) {
    if (row >= 0 && row < 4 && col >= 0 && col < 4) {
        cout << "🔘 按键: " << keyMap[row][col] << " (Row: " << row << ", Col: " << col << ")" << endl;
    } else {
        cerr << "⚠️ 无效的按键坐标: [" << row << ", " << col << "]" << endl;
    }
}

KeyboardEventHandler::KeyboardEventHandler(Keyboard* parent, int pin) 
    : parent(parent), associatedPin(pin) {}

void KeyboardEventHandler::handleEvent(const gpiod_line_event& event) {
    // 获取触发引脚
    struct gpiod_line* line = event.line;
    int pin_number = gpiod_line_get_offset(line);
    if (pin_number != associatedPin) {
        cerr << "关联引脚不匹配。" << endl;
        return;
    }

    // 去抖: 等待20ms以确保稳定
    this_thread::sleep_for(chrono::milliseconds(20));

    // 确认列引脚仍为低
    if (gpio.readGPIO(associatedPin) != 0) {
        return; // 按键在去抖期间释放
    }

    // 获取列索引
    int colIndex = -1;
    for (int i = 0; i < 4; i++) {
        if (colPins[i] == associatedPin) {
            colIndex = i;
            break;
        }
    }
    if (colIndex == -1) {
        cerr << "无效的列引脚: " << associatedPin << endl;
        return;
    }

    // 扫描行以找到按下的键
    for (int row = 0; row < 4; row++) {
        // 将当前行设置为高电平
        gpio.writeGPIO(rowPins[row], 1);
        this_thread::sleep_for(chrono::microseconds(10));

        // 检查列是否为高电平
        if (gpio.readGPIO(associatedPin) == 1) {
            // 键在此行-列交点被按下
            parent->processKeyPress(row, colIndex);
            // 重置行为低电平
            gpio.writeGPIO(rowPins[row], 0);
            return;
        }

        // 重置行为低电平
        gpio.writeGPIO(rowPins[row], 0);
    }
}

/////////////////////////////

#include <iostream>
#include <thread>
#include <chrono>
#include "gpio/gpio.h"
#include "pir/pir.h"
#include "dht/dht.h"
#include "display/i2c_display.h"
#include "i2c_handle.h"
#include "Keyboard/keyboard.h"
#include <string>

// 密码验证类
class PasswordHandler {
public:
    PasswordHandler() : correctPassword("1234#"), inputPassword(""), isLocked(true) {}
    
    void handleKeyPress(char key) {
        if (!isLocked) return; // 如果已解锁，忽略按键
        
        std::cout << "按下: " << key << std::endl;
        
        if (key == '#') {
            // '#' 作为确认键，检查密码
            if (inputPassword == correctPassword.substr(0, correctPassword.length() - 1)) {
                unlock();
            } else {
                wrongPassword();
            }
            inputPassword = ""; // 清空输入
        } else if (key == '*') {
            // '*' 作为清除键
            inputPassword = "";
            std::cout << "🔄 已清除输入" << std::endl;
        } else {
            // 其他键作为密码输入
            inputPassword += key;
        }
    }
    
    void unlock() {
        isLocked = false;
        std::cout << "🔓 密码正确！已解锁" << std::endl;
        // 添加解锁后操作
        
        // 10秒后自动锁定
        std::thread([this]() {
            std::this_thread::sleep_for(std::chrono::seconds(10));
            lock();
        }).detach();
    }
    
    void lock() {
        isLocked = true;
        inputPassword = "";
        std::cout << "🔒 已锁定" << std::endl;
    }
    
    void wrongPassword() {
        std::cout << "❌ 密码错误!" << std::endl;
        // 可以在这里添加错误提示，如蜂鸣器
    }
    
private:
    std::string correctPassword;
    std::string inputPassword;
    bool isLocked;
};

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

    // 创建 I2cDisplayHandle 实例，负责处理 PIR 与 DHT 事件
    I2cDisplayHandle displayHandle;
    gpio.registerCallback(PIR_IO, &displayHandle);

    // 启动 GPIO 事件监听线程
    gpio.start();

    // 初始化 DHT11 温湿度传感器，并注册回调，将数据传递给 I2cDisplayHandle 处理
    DHT11 dht11(gpio);
    dht11.registerCallback([&displayHandle](const DHTReading &reading) {
        displayHandle.handleDHT(reading.temp_celsius, reading.humidity);
    });
    dht11.start();

    // 创建密码处理器
    PasswordHandler passwordHandler;
    
    // 初始化矩阵键盘
    Keyboard keyboard(gpio);
    keyboard.init();
    
    // 重写 Keyboard 的 processKeyPress 方法，连接到密码处理器
    auto originalProcessKeyPress = keyboard.processKeyPress;
    keyboard.processKeyPress = [&passwordHandler, originalProcessKeyPress](int row, int col) {
        originalProcessKeyPress(row, col);
        if (row >= 0 && row < 4 && col >= 0 && col < 4) {
            passwordHandler.handleKeyPress(keyMap[row][col]);
        }
    };
    
    std::cout << "🔒 安全系统已启动，请输入密码解锁..." << std::endl;

    // 主循环保持运行
    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    // 释放资源
    keyboard.cleanup();
    gpio.stop();
    std::cout << "退出程序。" << std::endl;
    return 0;
}
