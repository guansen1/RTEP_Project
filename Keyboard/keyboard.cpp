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
#include "i2c_display.h"
#include <iostream>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>
#include <cstring>
#include <cerrno>
#include <cstdlib>
#include <chrono>

// **矩阵键盘 GPIO 引脚定义**
const int rowPins[4] = {KB_R1_IO, KB_R2_IO, KB_R3_IO, KB_R4_IO}; // 行（事件触发）
const int colPins[4] = {KB_R5_IO, KB_R6_IO, KB_R7_IO, KB_R8_IO}; // 列（事件触发）

using namespace std;

Keyboard::Keyboard(GPIO& gpio) : gpio(gpio), keyDetected(false) {
    lastPressTime = std::chrono::steady_clock::now();
}

Keyboard::~Keyboard() {
    cleanup();
}

void Keyboard::init() {
    std::cout << "⌨️ 初始化键盘 GPIO..." << std::endl;
    
    // 配置行为输入并使用内部上拉
    for (int i = 0; i < 4; i++) {
        std::cout << "配置行引脚 " << rowPins[i] << " 为下降沿触发" << std::endl;
        if (!gpio.configGPIO(rowPins[i], INPUT_PULLUP)) {
            std::cerr << "❌ 行引脚 " << rowPins[i] << " 配置失败！" << std::endl;
            continue;
        }
        auto* handler = new KeyboardEventHandler(this, rowPins[i]);
        if (!gpio.configGPIO(rowPins[i], FALLING_EDGE)) {
            std::cerr << "❌ 行引脚 " << rowPins[i] << " 配置为下降沿触发失败！" << std::endl;
            delete handler;
            continue;
        }
        gpio.registerCallback(rowPins[i], handler);
        handlers.push_back(handler);
        std::cout << "✅ 行引脚 " << rowPins[i] << " 配置成功" << std::endl;
    }
    
    // 配置列为输入
    for (int i = 0; i < 4; i++) {
        std::cout << "配置列引脚 " << colPins[i] << " 为上升沿触发" << std::endl;
        if (!gpio.configGPIO(colPins[i], INPUT)) {
            std::cerr << "❌ 列引脚 " << colPins[i] << " 配置失败！" << std::endl;
            continue;
        }
        auto* handler = new KeyboardEventHandler(this, colPins[i]);
        if (!gpio.configGPIO(colPins[i], RISING_EDGE)) {
            std::cerr << "❌ 列引脚 " << colPins[i] << " 配置为上升沿触发失败！" << std::endl;
            delete handler;
            continue;
        }
        gpio.registerCallback(colPins[i], handler);
        handlers.push_back(handler);
        std::cout << "✅ 列引脚 " << colPins[i] << " 配置成功" << std::endl;
    }
    
    std::cout << "✅ 键盘初始化完成" << std::endl;
}

void Keyboard::cleanup() {
    cout << "🔚 释放键盘 GPIO 资源" << endl;
    for (auto* handler : handlers) {
        delete handler;
    }
    handlers.clear();
}

void Keyboard::defaultProcessKeyPress(int row, int col) {
    if (row >= 0 && row < 4 && col >= 0 && col < 4) {
        cout << "🔘 按键: " << keyMap[row][col] << endl;
        
        // 添加密码检测逻辑
        //  '1', '2', '3', '4', '#' 解锁
        
        // 发送按键信息到其他系统组件
    } else {
        cerr << "⚠️ 无效的按键坐标: [" << row << ", " << col << "]" << endl;
    }
}

// 键盘事件处理器实现
KeyboardEventHandler::KeyboardEventHandler(Keyboard* parent, int pin) 
    : parent(parent), associatedPin(pin) {
}

void KeyboardEventHandler::handleEvent(const gpiod_line_event& event) {
    std::cout << "键盘事件触发：引脚 " << associatedPin 
              << (event.event_type == GPIOD_LINE_EVENT_RISING_EDGE ? " 上升沿" : " 下降沿") 
              << " 时间戳: " << event.ts.tv_sec << "." << event.ts.tv_nsec << std::endl;
              
    static int lastRow = -1, lastCol = -1;
    auto now = chrono::steady_clock::now();
    auto timeSinceLastPress = chrono::duration_cast<chrono::milliseconds>(
        now - parent->lastPressTime).count();
    
    // 消抖处理：忽略50ms内的连续触发
    if (timeSinceLastPress < 50) {
        std::cout << "忽略过快的连续触发（" << timeSinceLastPress << "ms）" << std::endl;
        return;
    }
    
    // 确定当前触发的是行还是列
    int rowIndex = -1, colIndex = -1;
    
    // 检查是否为行引脚
    for (int i = 0; i < 4; i++) {
        if (rowPins[i] == associatedPin) {
            rowIndex = i;
            std::cout << "检测到行 " << i << " 被按下" << std::endl;
            break;
        }
    }
    
    // 检查是否为列引脚
    for (int i = 0; i < 4; i++) {
        if (colPins[i] == associatedPin) {
            colIndex = i;
            std::cout << "检测到列 " << i << " 被激活" << std::endl;
            break;
        }
    }
    
    // 更新最近一次按键的行或列
    if (rowIndex != -1) {
        parent->activeRow = rowIndex;
        std::cout << "更新活动行为 " << rowIndex << std::endl;
    } else if (colIndex != -1) {
        parent->activeCol = colIndex;
        std::cout << "更新活动列为 " << colIndex << std::endl;
    }
    
    // 如果行和列都已确定，则处理按键
    if (parent->activeRow != -1 && parent->activeCol != -1 && 
        (parent->activeRow != lastRow || parent->activeCol != lastCol)) {
        
        std::cout << "完整按键检测：行=" << parent->activeRow 
                  << ", 列=" << parent->activeCol 
                  << ", 按键=" << keyMap[parent->activeRow][parent->activeCol] << std::endl;
        
        // 处理按键
        parent->processKeyPress(parent->activeRow, parent->activeCol);
        
        // 记录最后处理的按键
        lastRow = parent->activeRow;
        lastCol = parent->activeCol;
        
        // 更新时间戳
        parent->lastPressTime = now;
        
        // 按键处理后重置，等待下一次按键
        parent->activeRow = -1;
        parent->activeCol = -1;
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

// 自定义键盘处理器，连接键盘和密码验证
class CustomKeyboardHandler : public GPIO::GPIOEventCallbackInterface {
public:
    CustomKeyboardHandler(Keyboard* keyboard, PasswordHandler* passwordHandler) 
        : keyboard(keyboard), passwordHandler(passwordHandler) {
    }
    
    void handleEvent(const gpiod_line_event& event) override {
        // 这个处理器可以添加额外的逻辑，目前仅用于连接
    }
    
    // 设置为Keyboard的友元类，可以访问其私有方法
    friend class Keyboard;
    
private:
    Keyboard* keyboard;
    PasswordHandler* passwordHandler;
};

int main() {
    std::cout << "系统启动！" << std::endl;

    // 初始化 I2C 显示模块（SSD1306）
    I2cDisplay::getInstance().init();

    // 初始化 GPIO 模块
    GPIO gpio;
    gpio.gpio_init();

    // 注册原有的 PIR 事件处理器（用于日志输出等）
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
    
    // 重写Keyboard的processKeyPress方法，连接到密码处理器
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
