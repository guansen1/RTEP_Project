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


///////////////

void KeyboardEventHandler::handleEvent(const gpiod_line_event& event) {
    this_thread::sleep_for(chrono::milliseconds(20)); // Debounce
    if (parent->gpio.readGPIO(associatedPin) != 0) {
        return; // Key released during debounce
    }

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

    for (int row = 0; row < 4; row++) {
        parent->gpio.writeGPIO(rowPins[row], 1);
        this_thread::sleep_for(chrono::microseconds(10));
        if (parent->gpio.readGPIO(associatedPin) == 1) {
            parent->processKeyPress(row, colIndex);
            parent->gpio.writeGPIO(rowPins[row], 0);
            return;
        }
        parent->gpio.writeGPIO(rowPins[row], 0);
    }
}



///////
class Keyboard {
public:
    void processKeyPress(int row, int col); // Regular member function
    // Other declarations...
};


/////////////////////////

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

class Keyboard {
public:
    void processKeyPress(int row, int col); // Regular member function
    // Other declarations...
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



///////////



https://www.raylink.live/download.html

///////

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
    this_thread::sleep_for(chrono::milliseconds(20)); // Debounce
    if (parent->gpio.readGPIO(associatedPin) != 0) {
        return; // Key released during debounce
    }

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

    for (int row = 0; row < 4; row++) {
        parent->gpio.writeGPIO(rowPins[row], 1);
        this_thread::sleep_for(chrono::microseconds(10));
        if (parent->gpio.readGPIO(associatedPin) == 1) {
            parent->processKeyPress(row, colIndex);
            parent->gpio.writeGPIO(rowPins[row], 0);
            return;
        }
        parent->gpio.writeGPIO(rowPins[row], 0);
    }
}


//////////////////

//头文件 (keyboard.h)：#ifndef KEYBOARD_H
#define KEYBOARD_H

#include "gpiod.h"
#include "gpio/gpio.h"
#include <iostream>
#include <chrono>
#include <vector>

// 矩阵键盘 GPIO 引脚定义（外部定义）
extern const int rowPins[4]; // 行引脚，设置为输出
extern const int colPins[4]; // 列引脚，设置为输入并监听事件

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
    KeyboardEventHandler(class Keyboard* parent, int pin);
    void handleEvent(const gpiod_line_event& event) override;

private:
    Keyboard* parent;
    int associatedPin; // 保存关联的列引脚编号
};

// 键盘管理类
class Keyboard {
public:
    explicit Keyboard(GPIO& gpio);
    ~Keyboard();

    void init();
    void cleanup();
    void processKeyPress(int row, int col);
    void scanRowsAndProcess(int colIndex); // 新增扫描函数
    
    GPIO& getGPIO() { return gpio; }
    
private:
    GPIO& gpio;
    std::vector<KeyboardEventHandler*> handlers;
};

#endif // KEYBOARD_H

/////////////


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



//////////////////


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
/////////////////////////


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
    configGPIO(KB_R1_IO, FALLING_EDGE);
    configGPIO(KB_R2_IO, FALLING_EDGE);
    configGPIO(KB_R3_IO, FALLING_EDGE);
    configGPIO(KB_R4_IO, FALLING_EDGE);
    configGPIO(KB_R5_IO, RISING_EDGE);
    configGPIO(KB_R6_IO, RISING_EDGE);
    configGPIO(KB_R7_IO, RISING_EDGE);
    configGPIO(KB_R8_IO, RISING_EDGE);
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
        case RISING_EDGE: request_status = gpiod_line_request_rising_edge_events(line, "GPIO_edge_rising"); break;
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

void GPIO::registerCallback(int pin_number, GPIOEventCallbackInterface* callback, int event_type) {
    // 确保引脚已经配置为相应的事件类型
    if (gpio_config.find(pin_number) == gpio_config.end() || 
        (gpio_config[pin_number] != RISING_EDGE && 
         gpio_config[pin_number] != FALLING_EDGE && 
         gpio_config[pin_number] != BOTH_EDGES)) {
        
        // 如果引脚未配置或不是事件模式，配置它
        configGPIO(pin_number, event_type);
    }
    
    // 注册回调
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
        struct timespec timeout = {0, 100000000}; // 100ms超时，提高响应性

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
///////////

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
    configGPIO(KB_R1_IO, FALLING_EDGE);
    configGPIO(KB_R2_IO, FALLING_EDGE);
    configGPIO(KB_R3_IO, FALLING_EDGE);
    configGPIO(KB_R4_IO, FALLING_EDGE);
    configGPIO(KB_R5_IO, RISING_EDGE);
    configGPIO(KB_R6_IO, RISING_EDGE);
    configGPIO(KB_R7_IO, RISING_EDGE);
    configGPIO(KB_R8_IO, RISING_EDGE);
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
        case RISING_EDGE: request_status = gpiod_line_request_rising_edge_events(line, "GPIO_edge_rising"); break;
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

void GPIO::registerCallback(int pin_number, GPIOEventCallbackInterface* callback, int event_type) {
    // 确保引脚已经配置为相应的事件类型
    if (gpio_config.find(pin_number) == gpio_config.end() || 
        (gpio_config[pin_number] != RISING_EDGE && 
         gpio_config[pin_number] != FALLING_EDGE && 
         gpio_config[pin_number] != BOTH_EDGES)) {
        
        // 如果引脚未配置或不是事件模式，配置它
        configGPIO(pin_number, event_type);
    }
    
    // 注册回调
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
        struct timespec timeout = {0, 100000000}; // 100ms超时，提高响应性

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











