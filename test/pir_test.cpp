#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "gpio/gpio.h"
#include "pir/pir.h"
#include <chrono>
#include <thread>
#include <iostream>
#include <string>

// 创建一个 GPIO 的 Mock 类
class MockGPIO : public GPIO {
public:
    MOCK_METHOD(void, gpio_init, (), (override));
    MOCK_METHOD(bool, configGPIO, (int, int), (override));
    MOCK_METHOD(int, readGPIO, (int), (override));
    MOCK_METHOD(bool, writeGPIO, (int, int), (override));
    MOCK_METHOD(void, registerCallback, (int, GPIOEventCallbackInterface*), (override));
    MOCK_METHOD(void, start, (), (override));
    MOCK_METHOD(void, stop, (), (override));
};

// 测试夹具
class PIRTest : public ::testing::Test {
protected:
    void SetUp() override {
        // 设置测试环境
    }

    void TearDown() override {
        // 清理测试环境
    }

    MockGPIO mockGpio;
};

// 测试PIR初始化
TEST_F(PIRTest, Initialization) {
    // 创建PIR事件处理器
    PIREventHandler pirHandler(mockGpio);
    
    // 验证构造函数不会崩溃
    SUCCEED();
}

// 测试PIR处理上升沿事件
TEST_F(PIRTest, HandleRisingEdgeEvent) {
    // 创建PIR事件处理器
    PIREventHandler pirHandler(mockGpio);
    
    // 创建上升沿事件
    gpiod_line_event event;
    event.event_type = GPIOD_LINE_EVENT_RISING_EDGE;
    
    // 捕获标准输出
    testing::internal::CaptureStdout();
    
    // 处理事件
    pirHandler.handleEvent(event);
    
    // 获取输出并验证
    std::string output = testing::internal::GetCapturedStdout();
    EXPECT_THAT(output, testing::HasSubstr("[PIR] Triggered"));
}

// 测试PIR处理下降沿事件
TEST_F(PIRTest, HandleFallingEdgeEvent) {
    // 创建PIR事件处理器
    PIREventHandler pirHandler(mockGpio);
    
    // 创建下降沿事件
    gpiod_line_event event;
    event.event_type = GPIOD_LINE_EVENT_FALLING_EDGE;
    
    // 捕获标准输出
    testing::internal::CaptureStdout();
    
    // 处理事件
    pirHandler.handleEvent(event);
    
    // 获取输出并验证
    std::string output = testing::internal::GetCapturedStdout();
    EXPECT_THAT(output, testing::HasSubstr("[PIR] Trigger gone"));
}

// 测试PIR注册回调
TEST_F(PIRTest, RegisterCallback) {
    // 期望registerCallback方法被调用，并且传入的参数是PIR_IO和任意回调对象
    EXPECT_CALL(mockGpio, registerCallback(PIR_IO, testing::_))
        .Times(1);
    
    // 创建PIR事件处理器
    PIREventHandler pirHandler(mockGpio);
    
    // 手动注册回调
    mockGpio.registerCallback(PIR_IO, &pirHandler);
}

// 如果启用了实际硬件测试，添加一个与实际硬件交互的测试
#ifdef HARDWARE_TEST_ENABLED
TEST(PIRHardwareTest, TestWithRealHardware) {
    // 创建实际的GPIO对象
    GPIO gpio;
    
    // 初始化GPIO
    gpio.gpio_init();
    
    // 创建PIR事件处理器
    PIREventHandler pirHandler(gpio);
    
    // 注册回调
    gpio.registerCallback(PIR_IO, &pirHandler);
    
    // 启动GPIO事件监听
    gpio.start();
    
    std::cout << "PIR硬件测试：请在10秒内触发PIR传感器..." << std::endl;
    
    // 等待10秒，给用户时间触发传感器
    std::this_thread::sleep_for(std::chrono::seconds(10));
    
    // 停止GPIO事件监听
    gpio.stop();
    
    std::cout << "PIR硬件测试结束。" << std::endl;
    
    // 这个测试主要是提供一个与实际硬件交互的机会，所以不做断言
    SUCCEED();
}
#endif

int main(int argc, char **argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}