#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "gpio/gpio.h"

// 创建用于测试的模拟GPIO类
class MockGPIO : public GPIO {
public:
    // 使用构造函数覆盖父类构造函数，避免真实硬件访问
    MockGPIO() {
        // 注意：这不会调用父类构造函数
    }

    MOCK_METHOD(void, gpio_init, (), (override));
    MOCK_METHOD(bool, configGPIO, (int pin_number, int config_num), (override));
    MOCK_METHOD(int, readGPIO, (int pin_number), (override));
    MOCK_METHOD(bool, writeGPIO, (int pin_number, int value), (override));
    MOCK_METHOD(void, registerCallback, (int pin_number, GPIOEventCallbackInterface* callback), (override));
    MOCK_METHOD(void, start, (), (override));
    MOCK_METHOD(void, stop, (), (override));
};

// 测试回调类
class TestCallback : public GPIO::GPIOEventCallbackInterface {
public:
    MOCK_METHOD(void, handleEvent, (const gpiod_line_event& event), (override));
};

// 测试GPIO初始化
TEST(GPIOTest, TestInitialization) {
#ifdef HARDWARE_TEST_ENABLED
    // 使用真实GPIO测试，需要硬件支持
    GPIO gpio;
    gpio.gpio_init();
    
    // 验证初始化后的状态
    EXPECT_TRUE(gpio.readGPIO(PIR_IO) >= 0);
    EXPECT_TRUE(gpio.writeGPIO(BUZZER_IO, 0));
    EXPECT_TRUE(gpio.writeGPIO(DHT_IO, 0));
#else
    // 使用Mock测试，不需要硬件
    MockGPIO gpio;
    
    EXPECT_CALL(gpio, configGPIO(PIR_IO, BOTH_EDGES)).WillOnce(testing::Return(true));
    EXPECT_CALL(gpio, configGPIO(BUZZER_IO, OUTPUT)).WillOnce(testing::Return(true));
    EXPECT_CALL(gpio, configGPIO(DHT_IO, OUTPUT)).WillOnce(testing::Return(true));
    
    gpio.gpio_init();
#endif
}

// 测试GPIO配置
TEST(GPIOTest, TestConfigGPIO) {
#ifdef HARDWARE_TEST_ENABLED
    GPIO gpio;
    
    EXPECT_TRUE(gpio.configGPIO(PIR_IO, INPUT));
    EXPECT_TRUE(gpio.configGPIO(BUZZER_IO, OUTPUT));
    
    // 测试无效配置
    EXPECT_FALSE(gpio.configGPIO(PIR_IO, 99));
#else
    MockGPIO gpio;
    
    EXPECT_CALL(gpio, configGPIO(PIR_IO, INPUT)).WillOnce(testing::Return(true));
    EXPECT_CALL(gpio, configGPIO(BUZZER_IO, OUTPUT)).WillOnce(testing::Return(true));
    EXPECT_CALL(gpio, configGPIO(PIR_IO, 99)).WillOnce(testing::Return(false));
    
    EXPECT_TRUE(gpio.configGPIO(PIR_IO, INPUT));
    EXPECT_TRUE(gpio.configGPIO(BUZZER_IO, OUTPUT));
    EXPECT_FALSE(gpio.configGPIO(PIR_IO, 99));
#endif
}

// 测试GPIO读操作
TEST(GPIOTest, TestReadGPIO) {
#ifdef HARDWARE_TEST_ENABLED
    GPIO gpio;
    gpio.configGPIO(PIR_IO, INPUT);
    
    // 读取输入引脚，无法预测具体值，但应该是合法的
    EXPECT_GE(gpio.readGPIO(PIR_IO), 0);
    
    // 测试读取不存在的引脚
    EXPECT_EQ(gpio.readGPIO(100), -1);
#else
    MockGPIO gpio;
    
    EXPECT_CALL(gpio, readGPIO(PIR_IO)).WillOnce(testing::Return(1));
    EXPECT_CALL(gpio, readGPIO(100)).WillOnce(testing::Return(-1));
    
    EXPECT_EQ(gpio.readGPIO(PIR_IO), 1);
    EXPECT_EQ(gpio.readGPIO(100), -1);
#endif
}

// 测试GPIO写操作
TEST(GPIOTest, TestWriteGPIO) {
#ifdef HARDWARE_TEST_ENABLED
    GPIO gpio;
    gpio.configGPIO(BUZZER_IO, OUTPUT);
    
    // 测试输出高低电平
    EXPECT_TRUE(gpio.writeGPIO(BUZZER_IO, 1));
    EXPECT_TRUE(gpio.writeGPIO(BUZZER_IO, 0));
    
    // 测试写入无效引脚
    EXPECT_FALSE(gpio.writeGPIO(100, 1));
#else
    MockGPIO gpio;
    
    EXPECT_CALL(gpio, writeGPIO(BUZZER_IO, 1)).WillOnce(testing::Return(true));
    EXPECT_CALL(gpio, writeGPIO(BUZZER_IO, 0)).WillOnce(testing::Return(true));
    EXPECT_CALL(gpio, writeGPIO(100, 1)).WillOnce(testing::Return(false));
    
    EXPECT_TRUE(gpio.writeGPIO(BUZZER_IO, 1));
    EXPECT_TRUE(gpio.writeGPIO(BUZZER_IO, 0));
    EXPECT_FALSE(gpio.writeGPIO(100, 1));
#endif
}

// 测试回调注册
TEST(GPIOTest, TestRegisterCallback) {
#ifdef HARDWARE_TEST_ENABLED
    // 硬件测试中，只是简单验证不会崩溃
    GPIO gpio;
    TestCallback callback;
    
    gpio.configGPIO(PIR_IO, BOTH_EDGES);
    gpio.registerCallback(PIR_IO, &callback);
    
    // 启动短时间，验证不会崩溃
    gpio.start();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    gpio.stop();
#else
    MockGPIO gpio;
    TestCallback callback;
    
    EXPECT_CALL(gpio, registerCallback(PIR_IO, &callback));
    
    gpio.registerCallback(PIR_IO, &callback);
#endif
}

// 测试启动和停止
TEST(GPIOTest, TestStartStop) {
#ifdef HARDWARE_TEST_ENABLED
    GPIO gpio;
    
    // 简单验证启动停止不会崩溃
    gpio.start();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    gpio.stop();
#else
    MockGPIO gpio;
    
    EXPECT_CALL(gpio, start());
    EXPECT_CALL(gpio, stop());
    
    gpio.start();
    gpio.stop();
#endif
}

// 测试边缘触发
TEST(GPIOTest, TestEdgeTrigger) {
#ifndef HARDWARE_TEST_ENABLED
    // 这个测试只在非硬件模式下运行
    MockGPIO gpio;
    TestCallback callback;
    
    // 设置回调期望
    EXPECT_CALL(gpio, configGPIO(PIR_IO, BOTH_EDGES)).WillOnce(testing::Return(true));
    EXPECT_CALL(gpio, registerCallback(PIR_IO, &callback));
    EXPECT_CALL(callback, handleEvent(testing::_)).Times(0);  // 不期望在这个测试中被调用
    
    // 配置GPIO
    EXPECT_TRUE(gpio.configGPIO(PIR_IO, BOTH_EDGES));
    gpio.registerCallback(PIR_IO, &callback);
#endif
}

// 主函数
int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}