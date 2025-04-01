#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "keyboard/keyboard.h"
#include "gpio/gpio.h"
#include <chrono>
#include <thread>
#include <atomic>

// 创建一个Mock GPIO类，用于单元测试
class MockGPIO : public GPIO {
public:
    // 模拟GPIO状态的内部存储
    std::unordered_map<int, int> pinValues;
    
    // 记录方法调用，用于验证
    struct CallRecord {
        std::string method;
        int pin;
        int value;
    };
    std::vector<CallRecord> callHistory;
    
    // 构造函数
    MockGPIO() {
        // 初始化所有引脚为高电平（未按下状态）
        const int rowPins[4] = {13, 16, 20, 21};
        const int colPins[4] = {1, 7, 8, 26};
        
        for (int i = 0; i < 4; i++) {
            pinValues[rowPins[i]] = 1;
            pinValues[colPins[i]] = 1;
        }
    }
    
    ~MockGPIO() override {}
    
    void gpio_init() override {
        callHistory.push_back({"gpio_init", 0, 0});
    }
    
    // 重写GPIO方法，使其完全可控
    bool configGPIO(int pin_number, int config_num) override {
        callHistory.push_back({"configGPIO", pin_number, config_num});
        return true;
    }
    
    int readGPIO(int pin_number) override {
        callHistory.push_back({"readGPIO", pin_number, 0});
        // 如果引脚值存在则返回该值，否则返回默认值1
        return pinValues.count(pin_number) ? pinValues[pin_number] : 1;
    }
    
    bool writeGPIO(int pin_number, int value) override {
        callHistory.push_back({"writeGPIO", pin_number, value});
        pinValues[pin_number] = value;
        return true;
    }
    
    void registerCallback(int pin_number, GPIOEventCallbackInterface* callback) override {
        callHistory.push_back({"registerCallback", pin_number, 0});
    }
    
    void start() override {
        callHistory.push_back({"start", 0, 0});
    }
    
    void stop() override {
        callHistory.push_back({"stop", 0, 0});
    }
    
    // 辅助测试方法
    void clearHistory() {
        callHistory.clear();
    }
    
    // 设置引脚状态，用于模拟按键
    void setPinValue(int pin, int value) {
        pinValues[pin] = value;
    }
    
    // 模拟按下特定行列位置的按键
    void simulateKeyPress(int row, int col) {
        // 获取行列引脚编号
        const int rowPins[4] = {13, 16, 20, 21};
        // 设置对应行引脚为低电平（表示按下）
        pinValues[rowPins[row]] = 0;
    }
    
    // 模拟释放所有按键
    void simulateAllKeysReleased() {
        const int rowPins[4] = {13, 16, 20, 21};
        const int colPins[4] = {1, 7, 8, 26};
        
        for (int i = 0; i < 4; i++) {
            pinValues[rowPins[i]] = 1;
            pinValues[colPins[i]] = 1;
        }
    }
};

// 单元测试夹具类
class KeyboardUnitTest : public ::testing::Test {
protected:
    MockGPIO mockGpio;
    std::unique_ptr<ActiveKeyboardScanner> scanner;
    std::vector<char> detectedKeys;
    std::atomic<bool> callbackCalled;
    
    void SetUp() override {
        scanner = std::make_unique<ActiveKeyboardScanner>(mockGpio);
        detectedKeys.clear();
        callbackCalled = false;
        
        // 设置键盘回调函数
        scanner->setKeyCallback([this](char key) {
            detectedKeys.push_back(key);
            callbackCalled = true;
        });
    }
    
    void TearDown() override {
        if (scanner) {
            scanner->stop();
            scanner.reset();
        }
    }
    
    // 等待回调被调用或超时
    bool waitForCallback(int timeoutMs = 500) {
        auto start = std::chrono::steady_clock::now();
        while (!callbackCalled) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            auto now = std::chrono::steady_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - start).count();
            if (elapsed > timeoutMs) {
                return false; // 超时
            }
        }
        return true; // 回调被调用
    }
};

// 测试键盘扫描器的初始化
TEST_F(KeyboardUnitTest, Initialization) {
    // 验证构造函数不会导致崩溃
    ASSERT_NO_THROW({
        ActiveKeyboardScanner testScanner(mockGpio);
    });
}

// 测试开始和停止扫描
TEST_F(KeyboardUnitTest, StartAndStop) {
    // 开始扫描前清除历史记录
    mockGpio.clearHistory();
    
    // 开始扫描
    scanner->start();
    
    // 验证至少有一些GPIO写入操作
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    // 停止扫描
    scanner->stop();
    
    // 验证GPIO写入被调用（至少一次，设置列状态）
    bool writeGpioCalled = false;
    for (const auto& call : mockGpio.callHistory) {
        if (call.method == "writeGPIO") {
            writeGpioCalled = true;
            break;
        }
    }
    EXPECT_TRUE(writeGpioCalled);
}

// 测试按键检测
TEST_F(KeyboardUnitTest, KeyDetection) {
    // 开始扫描
    scanner->start();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    // 模拟按下第一行第一列的按键（'1'）
    mockGpio.simulateKeyPress(0, 0);
    
    // 等待回调被调用
    EXPECT_TRUE(waitForCallback(1000));
    
    // 验证检测到的按键是否正确
    ASSERT_FALSE(detectedKeys.empty());
    EXPECT_EQ(detectedKeys[0], '1');
    
    // 模拟释放按键
    callbackCalled = false;
    detectedKeys.clear();
    mockGpio.simulateAllKeysReleased();
    
    // 停止扫描
    scanner->stop();
}

// 测试多个按键序列
TEST_F(KeyboardUnitTest, KeySequence) {
    // 开始扫描
    scanner->start();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    // 模拟按下第一行第一列的按键（'1'）
    mockGpio.simulateKeyPress(0, 0);
    
    // 等待回调被调用
    EXPECT_TRUE(waitForCallback(1000));
    
    // 验证检测到的按键是否正确
    ASSERT_FALSE(detectedKeys.empty());
    EXPECT_EQ(detectedKeys[0], '1');
    
    // 模拟释放按键
    callbackCalled = false;
    mockGpio.simulateAllKeysReleased();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    // 模拟按下第二行第二列的按键（'5'）
    mockGpio.simulateKeyPress(1, 1);
    detectedKeys.clear();
    callbackCalled = false;
    
    // 等待回调被调用
    EXPECT_TRUE(waitForCallback(1000));
    
    // 验证检测到的按键是否正确
    ASSERT_FALSE(detectedKeys.empty());
    EXPECT_EQ(detectedKeys[0], '5');
    
    // 模拟释放按键
    mockGpio.simulateAllKeysReleased();
    
    // 停止扫描
    scanner->stop();
}

// 测试回调函数
TEST_F(KeyboardUnitTest, CallbackFunction) {
    bool testCallbackCalled = false;
    char detectedKey = '\0';
    
    // 设置新的回调函数
    scanner->setKeyCallback([&testCallbackCalled, &detectedKey](char key) {
        testCallbackCalled = true;
        detectedKey = key;
    });
    
    // 开始扫描
    scanner->start();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    // 模拟按下第三行第三列的按键（'9'）
    mockGpio.simulateKeyPress(2, 2);
    
    // 等待一段时间让回调有机会被调用
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    
    // 验证回调是否被调用且按键是否正确
    EXPECT_TRUE(testCallbackCalled);
    EXPECT_EQ(detectedKey, '9');
    
    // 停止扫描
    scanner->stop();
}