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



/////gpio.h



#ifndef GPIO_H
#define GPIO_H

#include <gpiod.h>
#include <iostream>
#include <unordered_map>
#include <vector>
#include <thread>
#include <atomic>
#include <memory>

enum GPIOconfig{
    INPUT = 0,          // 输入模式
    OUTPUT = 1,         // 输出模式
    INPUT_PULLUP = 2,   // 上拉输入
    INPUT_PULLDOWN = 3, // 下拉输入
    RISING_EDGE = 4,    // 上升沿触发事件
    FALLING_EDGE = 5,   // 下降沿触发事件
    BOTH_EDGES = 6      // 双边沿触发事件
};

enum GPIOdef{
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
    bool configGPIO(int pin_number, int config_num);
    int readGPIO(int pin_number);
    bool writeGPIO(int pin_number, int value);
    void registerCallback(int pin_number, GPIOEventCallbackInterface* callback);
    void registerCallback(int pin_number, GPIOEventCallbackInterface* callback, int event_type);
    void start();
    void stop();
    struct gpiod_chip* getChip() { return chip; }  // 添加getter方法

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
////  keyboard.cpp

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
    cout << "⌨️ 初始化键盘 GPIO..." << endl;
    
    // 配置行为输入并使用内部上拉
    for (int i = 0; i < 4; i++) {
        auto* handler = new KeyboardEventHandler(this, rowPins[i]);
        gpio.configGPIO(rowPins[i], INPUT_PULLUP);
        gpio.registerCallback(rowPins[i], handler);
        handlers.push_back(handler);
    }
    
    // 配置列为输入
    for (int i = 0; i < 4; i++) {
        auto* handler = new KeyboardEventHandler(this, colPins[i]);
        gpio.configGPIO(colPins[i], INPUT);
        gpio.registerCallback(colPins[i], handler);
        handlers.push_back(handler);
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
        cout << "🔘 按键: " << keyMap[row][col] << endl;
        
        // 这里可以添加密码检测逻辑
        // 例如: 如果按下的是 '1', '2', '3', '4', '#' 则解锁
        
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
    static int lastRow = -1, lastCol = -1;
    auto now = chrono::steady_clock::now();
    auto timeSinceLastPress = chrono::duration_cast<chrono::milliseconds>(
        now - parent->lastPressTime).count();
    
    // 消抖处理：忽略50ms内的连续触发
    if (timeSinceLastPress < 50) {
        return;
    }
    
    // 确定当前触发的是行还是列
    int rowIndex = -1, colIndex = -1;
    
    // 检查是否为行引脚
    for (int i = 0; i < 4; i++) {
        if (rowPins[i] == associatedPin) {
            rowIndex = i;
            break;
        }
    }
    
    // 检查是否为列引脚
    for (int i = 0; i < 4; i++) {
        if (colPins[i] == associatedPin) {
            colIndex = i;
            break;
        }
    }
    
    // 更新最近一次按键的行或列
    if (rowIndex != -1) {
        parent->activeRow = rowIndex;
    } else if (colIndex != -1) {
        parent->activeCol = colIndex;
    }
    
    // 如果行和列都已确定，则处理按键
    if (parent->activeRow != -1 && parent->activeCol != -1 && 
        (parent->activeRow != lastRow || parent->activeCol != lastCol)) {
        
        // 处理按键
        parent->processKeyPress(parent->activeRow, parent->activeCol);
        
        // 记录最后处理的按键
        lastRow = parent->activeRow;
        lastCol = parent->activeCol;
        
        // 更新时间戳
        parent->lastPressTime = now;
        
        // 按键处理后重置，等待下一次按键
        // parent->activeRow = -1;
        // parent->activeCol = -1;
    }
}


///keyboard.cpp/////

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
    cout << "⌨️ 初始化键盘 GPIO..." << endl;
    
    // 配置行为输入并使用内部上拉
    for (int i = 0; i < 4; i++) {
        auto* handler = new KeyboardEventHandler(this, rowPins[i]);
        gpio.configGPIO(rowPins[i], INPUT_PULLUP);
        gpio.registerCallback(rowPins[i], handler);
        handlers.push_back(handler);
    }
    
    // 配置列为输入
    for (int i = 0; i < 4; i++) {
        auto* handler = new KeyboardEventHandler(this, colPins[i]);
        gpio.configGPIO(colPins[i], INPUT);
        gpio.registerCallback(colPins[i], handler);
        handlers.push_back(handler);
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
        cout << "🔘 按键: " << keyMap[row][col] << endl;
        
        // 这里可以添加密码检测逻辑
        // 例如: 如果按下的是 '1', '2', '3', '4', '#' 则解锁
        
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
    static int lastRow = -1, lastCol = -1;
    auto now = chrono::steady_clock::now();
    auto timeSinceLastPress = chrono::duration_cast<chrono::milliseconds>(
        now - parent->lastPressTime).count();
    
    // 消抖处理：忽略50ms内的连续触发
    if (timeSinceLastPress < 50) {
        return;
    }
    
    // 确定当前触发的是行还是列
    int rowIndex = -1, colIndex = -1;
    
    // 检查是否为行引脚
    for (int i = 0; i < 4; i++) {
        if (rowPins[i] == associatedPin) {
            rowIndex = i;
            break;
        }
    }
    
    // 检查是否为列引脚
    for (int i = 0; i < 4; i++) {
        if (colPins[i] == associatedPin) {
            colIndex = i;
            break;
        }
    }
    
    // 更新最近一次按键的行或列
    if (rowIndex != -1) {
        parent->activeRow = rowIndex;
    } else if (colIndex != -1) {
        parent->activeCol = colIndex;
    }
    
    // 如果行和列都已确定，则处理按键
    if (parent->activeRow != -1 && parent->activeCol != -1 && 
        (parent->activeRow != lastRow || parent->activeCol != lastCol)) {
        
        // 处理按键
        parent->processKeyPress(parent->activeRow, parent->activeCol);
        
        // 记录最后处理的按键
        lastRow = parent->activeRow;
        lastCol = parent->activeCol;
        
        // 更新时间戳
        parent->lastPressTime = now;
        
        // 按键处理后重置，等待下一次按键
        // parent->activeRow = -1;
        // parent->activeCol = -1;
    }
}


/////


#ifndef KEYBOARD_H
#define KEYBOARD_H

#include "gpiod.h"
#include "gpio/gpio.h"
#include <iostream>
#include <chrono>
#include <vector>
#include <functional>

// **矩阵键盘 GPIO 引脚定义**
extern const int rowPins[4]; // 行（事件触发）
extern const int colPins[4]; // 列（事件触发）

// 按键映射表
const char keyMap[4][4] = {
    {'1', '2', '3', 'A'},
    {'4', '5', '6', 'B'},
    {'7', '8', '9', 'C'},
    {'*', '0', '#', 'D'}
};

// 前向声明
class Keyboard;

// **键盘事件处理类**
class KeyboardEventHandler : public GPIO::GPIOEventCallbackInterface {
public:
    // 增加 pin 参数，用于保存当前回调关联的 GPIO 引脚编号
    KeyboardEventHandler(class Keyboard* parent, int pin);
    void handleEvent(const gpiod_line_event& event) override;

private:
    Keyboard* parent;
    int associatedPin; // 保存注册时传入的 GPIO 引脚编号
};

// **键盘管理类**
class Keyboard {
public:
    explicit Keyboard(GPIO& gpio);
    ~Keyboard();

    void init();
    void cleanup();
    
    // 使用std::function可以允许在运行时重新定义此方法
    std::function<void(int, int)> processKeyPress = [this](int row, int col) {
        this->defaultProcessKeyPress(row, col);
    };
    
    void defaultProcessKeyPress(int row, int col);

    GPIO& getGPIO() { return gpio; } // 提供访问 GPIO 的 public 方法

    // 让KeyboardEventHandler成为友元类，可以访问私有成员
    friend class KeyboardEventHandler;

private:
    GPIO& gpio;
    std::vector<KeyboardEventHandler*> handlers;
    
    // 当前检测到的活动行和列
    int activeRow = -1, activeCol = -1;
    bool keyDetected = false;
    std::chrono::steady_clock::time_point lastPressTime;
};

#endif // KEYBOARD_H
