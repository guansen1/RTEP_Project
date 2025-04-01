#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "display/i2c_display.h"
#include "display/i2c_handle.h"
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>
#include <thread>
#include <chrono>

// 创建一个I2cDisplay的Mock类用于自动化测试
class MockI2cDisplay : public I2cDisplay {
public:
    MOCK_METHOD(void, init, (), (override));
    MOCK_METHOD(void, displayText, (const std::string &text), (override));
    MOCK_METHOD(void, displayTextAt, (int page, const std::string &text), (override));
    MOCK_METHOD(void, displayMultiLine, (const std::string &line1, const std::string &line2), (override));
    MOCK_METHOD(void, displayIntrusion, (), (override));
    MOCK_METHOD(void, displaySafe, (), (override));
    MOCK_METHOD(void, displaySafeAndDHT, (const std::string &tempStr, const std::string &humStr), (override));
    MOCK_METHOD(void, registerEventCallback, (std::function<void(const std::string&)> callback), (override));
    
    // 暴露私有方法进行测试
    void exposedSendCommand(uint8_t cmd) { sendCommand(cmd); }
    void exposedSendBuffer(const uint8_t* buf, size_t len) { sendBuffer(buf, len); }
    void exposedClearBuffer() { clearBuffer(); }
    int exposedTextWidth(const std::string &text) { return textWidth(text); }
    void exposedDrawChar(int x, char c) { drawChar(x, c); }
    void exposedDrawCharAt(int x, int page, char c) { drawCharAt(x, page, c); }
};

// 为了使用单例模式的测试技巧
class MockDisplaySingleton {
public:
    static MockI2cDisplay* mockInstance;
    
    static I2cDisplay& getMockInstance() {
        if (!mockInstance) {
            mockInstance = new MockI2cDisplay();
        }
        return *mockInstance;
    }
    
    static void cleanup() {
        if (mockInstance) {
            delete mockInstance;
            mockInstance = nullptr;
        }
    }
};

MockI2cDisplay* MockDisplaySingleton::mockInstance = nullptr;

// ==================== 自动化测试部分 ====================

// 自动化测试夹具
class I2cDisplayAutoTest : public ::testing::Test {
protected:
    void SetUp() override {
        // 保存原始的getInstance函数
        original_getInstance = I2cDisplay::getInstance;
        
        // 替换为我们的Mock版本
        // 注意：这种技术在正式产品中不推荐使用，但对于测试很有用
        I2cDisplay::getInstance = MockDisplaySingleton::getMockInstance;
    }
    
    void TearDown() override {
        // 恢复原始函数
        I2cDisplay::getInstance = original_getInstance;
        MockDisplaySingleton::cleanup();
    }
    
    // 保存原始的getInstance函数指针
    I2cDisplay& (*original_getInstance)() = nullptr;
};

// 测试文本宽度计算
TEST_F(I2cDisplayAutoTest, TextWidthCalculation) {
    MockI2cDisplay mockDisplay;
    EXPECT_EQ(mockDisplay.exposedTextWidth("A"), 6);
    EXPECT_EQ(mockDisplay.exposedTextWidth("ABC"), 18);
    EXPECT_EQ(mockDisplay.exposedTextWidth(""), 0);
}

// 测试单例模式
TEST_F(I2cDisplayAutoTest, GetInstanceReturnsSameInstance) {
    auto& instance1 = I2cDisplay::getInstance();
    auto& instance2 = I2cDisplay::getInstance();
    EXPECT_EQ(&instance1, &instance2);
}

// 测试入侵状态显示
TEST_F(I2cDisplayAutoTest, DisplayIntrusion) {
    auto mockDisplay = MockDisplaySingleton::mockInstance;
    
    // 期望调用displayText方法
    EXPECT_CALL(*mockDisplay, displayText("INVASION")).Times(1);
    
    // 测试回调功能
    bool callbackTriggered = false;
    std::string callbackMessage;
    mockDisplay->registerEventCallback([&callbackTriggered, &callbackMessage](const std::string& msg) {
        callbackTriggered = true;
        callbackMessage = msg;
    });
    
    // 调用目标方法
    mockDisplay->displayIntrusion();
    
    // 验证回调被触发
    EXPECT_TRUE(callbackTriggered);
    EXPECT_EQ(callbackMessage, "INVASION");
}

// 测试安全状态显示
TEST_F(I2cDisplayAutoTest, DisplaySafe) {
    auto mockDisplay = MockDisplaySingleton::mockInstance;
    
    // 期望调用displayText方法
    EXPECT_CALL(*mockDisplay, displayText("SAFE")).Times(1);
    
    // 测试回调功能
    bool callbackTriggered = false;
    std::string callbackMessage;
    mockDisplay->registerEventCallback([&callbackTriggered, &callbackMessage](const std::string& msg) {
        callbackTriggered = true;
        callbackMessage = msg;
    });
    
    // 调用目标方法
    mockDisplay->displaySafe();
    
    // 验证回调被触发
    EXPECT_TRUE(callbackTriggered);
    EXPECT_EQ(callbackMessage, "SAFE");
}

// 测试安全状态+DHT数据显示
TEST_F(I2cDisplayAutoTest, DisplaySafeAndDHT) {
    auto mockDisplay = MockDisplaySingleton::mockInstance;
    
    // 期望调用相关方法
    EXPECT_CALL(*mockDisplay, exposedClearBuffer()).Times(1);
    EXPECT_CALL(*mockDisplay, displayTextAt(0, "SAFE")).Times(1);
    EXPECT_CALL(*mockDisplay, displayTextAt(2, "Temp:25.5 C")).Times(1);
    EXPECT_CALL(*mockDisplay, displayTextAt(4, "Hum:60.3 %")).Times(1);
    EXPECT_CALL(*mockDisplay, exposedSendBuffer(testing::_, testing::_)).Times(1);
    
    // 调用目标方法
    mockDisplay->displaySafeAndDHT("Temp:25.5 C", "Hum:60.3 %");
}

// I2cDisplayHandle自动化测试
class I2cDisplayHandleAutoTest : public ::testing::Test {
protected:
    void SetUp() override {
        // 保存原始的getInstance函数
        original_getInstance = I2cDisplay::getInstance;
        
        // 替换为我们的Mock版本
        I2cDisplay::getInstance = MockDisplaySingleton::getMockInstance;
        
        // 创建测试对象
        handler = new I2cDisplayHandle();
    }
    
    void TearDown() override {
        delete handler;
        
        // 恢复原始函数
        I2cDisplay::getInstance = original_getInstance;
        MockDisplaySingleton::cleanup();
    }
    
    I2cDisplayHandle* handler;
    I2cDisplay& (*original_getInstance)() = nullptr;
};

// 测试处理上升沿事件（入侵状态）
TEST_F(I2cDisplayHandleAutoTest, HandleRisingEdgeEvent) {
    auto mockDisplay = MockDisplaySingleton::mockInstance;
    
    // 期望调用displayIntrusion方法
    EXPECT_CALL(*mockDisplay, displayIntrusion()).Times(1);
    
    // 模拟GPIO上升沿事件
    gpiod_line_event event;
    event.event_type = GPIOD_LINE_EVENT_RISING_EDGE;
    
    // 调用处理方法
    handler->handleEvent(event);
}

// 测试处理下降沿事件（安全状态）
TEST_F(I2cDisplayHandleAutoTest, HandleFallingEdgeEvent) {
    auto mockDisplay = MockDisplaySingleton::mockInstance;
    
    // 对于下降沿，不会立即调用displaySafe
    EXPECT_CALL(*mockDisplay, displaySafe()).Times(0);
    
    // 模拟GPIO下降沿事件
    gpiod_line_event event;
    event.event_type = GPIOD_LINE_EVENT_FALLING_EDGE;
    
    // 调用处理方法
    handler->handleEvent(event);
}

// 测试安全状态下处理DHT数据
TEST_F(I2cDisplayHandleAutoTest, HandleDHTDataInSafeState) {
    auto mockDisplay = MockDisplaySingleton::mockInstance;
    
    // 首先设置为安全状态
    gpiod_line_event event;
    event.event_type = GPIOD_LINE_EVENT_FALLING_EDGE;
    handler->handleEvent(event);
    
    // 期望调用displaySafeAndDHT方法
    EXPECT_CALL(*mockDisplay, displaySafeAndDHT(
        testing::StartsWith("Temp:25"), 
        testing::StartsWith("Hum:60")
    )).Times(1);
    
    // 调用DHT处理方法
    handler->handleDHT(25.5f, 60.3f);
}

// 测试入侵状态下处理DHT数据（不应更新显示）
TEST_F(I2cDisplayHandleAutoTest, HandleDHTDataInInvasionState) {
    auto mockDisplay = MockDisplaySingleton::mockInstance;
    
    // 首先设置为入侵状态
    gpiod_line_event event;
    event.event_type = GPIOD_LINE_EVENT_RISING_EDGE;
    handler->handleEvent(event);
    
    // 期望不会调用displaySafeAndDHT方法
    EXPECT_CALL(*mockDisplay, displaySafeAndDHT(testing::_, testing::_)).Times(0);
    
    // 调用DHT处理方法
    handler->handleDHT(25.5f, 60.3f);
}

// ==================== 硬件测试部分 ====================

// 检查是否启用了硬件测试
#ifdef HARDWARE_TEST_ENABLED

// 硬件测试夹具
class I2cDisplayHwTest : public ::testing::Test {
protected:
    void SetUp() override {
        // 初始化显示器
        I2cDisplay::getInstance().init();
    }

    void TearDown() override {
        // 测试结束后显示空白内容
        I2cDisplay::getInstance().displayText("");
    }
};

// 测试显示文本功能
TEST_F(I2cDisplayHwTest, DisplayText) {
    // 测试显示简单文本
    I2cDisplay::getInstance().displayText("Testing");
    // 暂停一会儿让你观察显示内容
    std::this_thread::sleep_for(std::chrono::seconds(2));
    
    // 测试较长文本
    I2cDisplay::getInstance().displayText("Hello World!");
    std::this_thread::sleep_for(std::chrono::seconds(2));
    
    SUCCEED() << "请目视确认文本是否正确显示";
}

// 测试多行文本显示
TEST_F(I2cDisplayHwTest, DisplayMultiLine) {
    I2cDisplay::getInstance().displayMultiLine("Line 1", "Line 2");
    std::this_thread::sleep_for(std::chrono::seconds(2));
    
    I2cDisplay::getInstance().displayMultiLine("Test", "MultiLine");
    std::this_thread::sleep_for(std::chrono::seconds(2));
    
    SUCCEED() << "请目视确认多行文本是否正确显示";
}

// 测试入侵状态显示
TEST_F(I2cDisplayHwTest, DisplayIntrusion) {
    // 测试回调功能
    bool callbackTriggered = false;
    std::string callbackMessage;
    
    I2cDisplay::getInstance().registerEventCallback([&callbackTriggered, &callbackMessage](const std::string& msg) {
        callbackTriggered = true;
        callbackMessage = msg;
    });
    
    // 显示入侵状态
    I2cDisplay::getInstance().displayIntrusion();
    std::this_thread::sleep_for(std::chrono::seconds(2));
    
    // 验证回调被正确触发
    EXPECT_TRUE(callbackTriggered);
    EXPECT_EQ(callbackMessage, "INVASION");
    SUCCEED() << "请目视确认是否显示 'INVASION'";
}

// 测试安全状态显示
TEST_F(I2cDisplayHwTest, DisplaySafe) {
    // 测试回调功能
    bool callbackTriggered = false;
    std::string callbackMessage;
    
    I2cDisplay::getInstance().registerEventCallback([&callbackTriggered, &callbackMessage](const std::string& msg) {
        callbackTriggered = true;
        callbackMessage = msg;
    });
    
    // 显示安全状态
    I2cDisplay::getInstance().displaySafe();
    std::this_thread::sleep_for(std::chrono::seconds(2));
    
    // 验证回调被正确触发
    EXPECT_TRUE(callbackTriggered);
    EXPECT_EQ(callbackMessage, "SAFE");
    SUCCEED() << "请目视确认是否显示 'SAFE'";
}

// 测试安全状态+温湿度显示
TEST_F(I2cDisplayHwTest, DisplaySafeAndDHT) {
    I2cDisplay::getInstance().displaySafeAndDHT("Temp:25.5 C", "Hum:60.3 %");
    std::this_thread::sleep_for(std::chrono::seconds(2));
    
    SUCCEED() << "请目视确认是否显示 'SAFE' 及温湿度数据";
}

// I2cDisplayHandle硬件测试夹具
class I2cDisplayHandleHwTest : public ::testing::Test {
protected:
    void SetUp() override {
        // 初始化
        handler = new I2cDisplayHandle();
    }

    void TearDown() override {
        delete handler;
        // 恢复显示器状态
        I2cDisplay::getInstance().displayText("");
    }
    
    I2cDisplayHandle* handler;
};

// 测试处理上升沿事件（入侵状态）
TEST_F(I2cDisplayHandleHwTest, HandleRisingEdgeEvent) {
    // 模拟GPIO上升沿事件（入侵）
    gpiod_line_event event;
    event.event_type = GPIOD_LINE_EVENT_RISING_EDGE;
    
    // 调用处理方法
    handler->handleEvent(event);
    
    // 暂停让你观察显示内容（应该显示INVASION）
    std::this_thread::sleep_for(std::chrono::seconds(2));
    
    SUCCEED() << "请目视确认是否显示 'INVASION'";
}

// 测试处理下降沿事件（安全状态）
TEST_F(I2cDisplayHandleHwTest, HandleFallingEdgeEvent) {
    // 首先设置入侵状态
    gpiod_line_event rising;
    rising.event_type = GPIOD_LINE_EVENT_RISING_EDGE;
    handler->handleEvent(rising);
    std::this_thread::sleep_for(std::chrono::seconds(1));
    
    // 然后发送下降沿事件（安全）
    gpiod_line_event falling;
    falling.event_type = GPIOD_LINE_EVENT_FALLING_EDGE;
    handler->handleEvent(falling);
    
    // 暂停（注意：此时不会显示SAFE，需要DHT数据更新才会显示）
    std::this_thread::sleep_for(std::chrono::seconds(1));
    
    SUCCEED() << "状态已转为SAFE，但需要DHT数据更新才会显示";
}

// 测试安全状态下处理DHT数据
TEST_F(I2cDisplayHandleHwTest, HandleDHTDataInSafeState) {
    // 首先设置为安全状态（通过下降沿事件）
    gpiod_line_event event;
    event.event_type = GPIOD_LINE_EVENT_FALLING_EDGE;
    handler->handleEvent(event);
    
    // 调用DHT处理方法
    handler->handleDHT(25.5f, 60.3f);
    
    // 暂停让你观察显示内容（应该显示SAFE和温湿度数据）
    std::this_thread::sleep_for(std::chrono::seconds(2));
    
    SUCCEED() << "请目视确认是否显示 'SAFE' 及温湿度数据";
}

// 测试入侵状态下处理DHT数据（不应更新显示）
TEST_F(I2cDisplayHandleHwTest, HandleDHTDataInInvasionState) {
    // 首先设置为入侵状态（通过上升沿事件）
    gpiod_line_event event;
    event.event_type = GPIOD_LINE_EVENT_RISING_EDGE;
    handler->handleEvent(event);
    std::this_thread::sleep_for(std::chrono::seconds(1));
    
    // 调用DHT处理方法
    handler->handleDHT(25.5f, 60.3f);
    
    // 暂停让你观察显示内容（应该仍然显示INVASION，不会显示温湿度）
    std::this_thread::sleep_for(std::chrono::seconds(2));
    
    SUCCEED() << "请目视确认是否仍显示 'INVASION'，而不是温湿度数据";
}

// 测试状态切换的完整流程
TEST_F(I2cDisplayHandleHwTest, CompleteStateTransitionFlow) {
    // 1. 初始状态（应为SAFE）
    std::cout << "初始状态（应为SAFE）" << std::endl;
    handler->handleDHT(22.0f, 55.0f);
    std::this_thread::sleep_for(std::chrono::seconds(2));
    
    // 2. 切换到入侵状态
    std::cout << "切换到入侵状态（INVASION）" << std::endl;
    gpiod_line_event rising;
    rising.event_type = GPIOD_LINE_EVENT_RISING_EDGE;
    handler->handleEvent(rising);
    std::this_thread::sleep_for(std::chrono::seconds(2));
    
    // 3. 在入侵状态下尝试更新DHT数据（不应生效）
    std::cout << "在入侵状态下更新DHT数据（应仍显示INVASION）" << std::endl;
    handler->handleDHT(23.0f, 56.0f);
    std::this_thread::sleep_for(std::chrono::seconds(2));
    
    // 4. 切换回安全状态
    std::cout << "切换回安全状态" << std::endl;
    gpiod_line_event falling;
    falling.event_type = GPIOD_LINE_EVENT_FALLING_EDGE;
    handler->handleEvent(falling);
    std::this_thread::sleep_for(std::chrono::seconds(1));
    
    // 5. 在安全状态下更新DHT数据
    std::cout << "在安全状态下更新DHT数据（应显示SAFE和温湿度）" << std::endl;
    handler->handleDHT(24.0f, 57.0f);
    std::this_thread::sleep_for(std::chrono::seconds(2));
    
    SUCCEED() << "请确认完整状态转换流程测试成功完成";
}

#endif // HARDWARE_TEST_ENABLED

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    
#ifdef HARDWARE_TEST_ENABLED
    std::cout << "===============================================" << std::endl;
    std::cout << "  硬件测试已启用！" << std::endl;
    std::cout << "  请确保SSD1306显示器已正确连接到I2C总线" << std::endl;
    std::cout << "  测试过程中，请目视观察显示内容是否正确" << std::endl;
    std::cout << "===============================================" << std::endl;
#else
    std::cout << "===============================================" << std::endl;
    std::cout << "  运行自动化测试，无需硬件" << std::endl;
    std::cout << "  如需运行硬件测试，请添加 -DENABLE_HARDWARE_TEST=ON" << std::endl;
    std::cout << "===============================================" << std::endl;
#endif
    
    return RUN_ALL_TESTS();
}