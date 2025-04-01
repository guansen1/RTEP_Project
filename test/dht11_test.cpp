#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <chrono>
#include <thread>
#include <vector>
#include <iostream>
#include "../dht/dht.h"
#include "../gpio/gpio.h"

// 创建模拟GPIO类，继承自您的GPIO类但重写关键方法
class MockGPIO : public GPIO {
public:
    // 使用GMock的宏来模拟方法
    MOCK_METHOD(bool, configGPIO, (int pin_number, int config_num), (override));
    MOCK_METHOD(bool, writeGPIO, (int pin_number, int value), (override));
    MOCK_METHOD(int, readGPIO, (int pin_number), (override));
    
    // 覆盖构造函数，防止访问硬件
    MockGPIO() {
        // 不调用父类构造函数中的硬件初始化代码
    }
    
    // 覆盖其他可能访问硬件的方法
    void gpio_init() override {}
    void start() override {}
    void stop() override {}
    void registerCallback(int pin_number, GPIO::GPIOEventCallbackInterface* callback) override {
        // 空实现，不调用实际硬件操作
    }
    
    // 覆盖析构函数，防止访问硬件
    ~MockGPIO() override {
        // 不调用父类析构函数中的硬件释放代码
    }
};

// 创建可控的GPIO测试类
class TestableGPIO : public GPIO {
public:
    TestableGPIO() {
        // 初始化状态，不访问实际硬件
        useSequence = false;
        
        // 初始化引脚状态数组
        for (int i = 0; i < 64; i++) {
            pinValues[i] = 0;
            pinDirections[i] = GPIOconfig::INPUT;
        }
    }
    
    // 重写configGPIO方法
    bool configGPIO(int pin_number, int config_num) override {
        if (pin_number < 64) {
            pinDirections[pin_number] = config_num;
            return true;
        }
        return false;
    }
    
    // 重写writeGPIO方法
    bool writeGPIO(int pin_number, int value) override {
        if (pin_number < 64) {
            pinValues[pin_number] = value;
            return true;
        }
        return false;
    }
    
    // 重写readGPIO方法
    int readGPIO(int pin_number) override {
        // 如果使用预设序列且是DHT引脚，则返回序列中的值
        if (useSequence && pin_number == GPIOdef::DHT_IO && sequenceIndex < readSequence.size()) {
            return readSequence[sequenceIndex++];
        }
        
        // 否则返回存储的引脚值
        if (pin_number < 64) {
            return pinValues[pin_number];
        }
        return -1; // 错误状态
    }
    
    // 设置读取序列，用于模拟DHT11通信
    void setReadSequence(const std::vector<int>& sequence) {
        readSequence = sequence;
        sequenceIndex = 0;
        useSequence = true;
    }
    
    // 重置序列索引
    void resetSequence() {
        sequenceIndex = 0;
    }
    
    // 禁用序列
    void disableSequence() {
        useSequence = false;
    }
    
    // 覆盖其他硬件相关方法
    void gpio_init() override {}
    void start() override {}
    void stop() override {}
    void registerCallback(int pin_number, GPIO::GPIOEventCallbackInterface* callback) override {}
    
    // 覆盖析构函数
    ~TestableGPIO() override {}
    
private:
    int pinValues[64];  // 存储引脚值
    int pinDirections[64];  // 存储引脚方向
    std::vector<int> readSequence;  // 预设的读取序列
    size_t sequenceIndex = 0;  // 当前序列索引
    bool useSequence;  // 是否使用序列
};

// 测试夹具类
class DHT11Test : public ::testing::Test {
protected:
    void SetUp() override {
        // 测试前的准备工作
    }
    
    void TearDown() override {
        // 测试后的清理工作
    }
    
    // 生成DHT11应答序列
    std::vector<int> generateDHT11ResponseSequence() {
        std::vector<int> sequence;
        
        // 1. 初始状态 (高电平)
        for (int i = 0; i < 5; i++) sequence.push_back(1);
        
        // 2. DHT11响应信号 (先拉低后拉高)
        for (int i = 0; i < 10; i++) sequence.push_back(0);  // 拉低 ~80us
        for (int i = 0; i < 10; i++) sequence.push_back(1);  // 拉高 ~80us
        
        return sequence;
    }
    
    // 生成一个完整的数据读取序列
    std::vector<int> generateDHT11DataSequence(const uint8_t data[5]) {
        std::vector<int> sequence = generateDHT11ResponseSequence();
        
        // 添加数据位 (5字节, 每字节8位)
        for (int byte = 0; byte < 5; byte++) {
            for (int bit = 7; bit >= 0; bit--) {
                // 位开始信号 (低电平 ~50us)
                for (int i = 0; i < 5; i++) sequence.push_back(0);
                
                // 数据位值 (高电平时长决定是0还是1)
                bool isOne = (data[byte] >> bit) & 0x01;
                int highDuration = isOne ? 12 : 5;  // 1~70us, 0~28us
                for (int i = 0; i < highDuration; i++) sequence.push_back(1);
                
                // 确保高电平结束
                sequence.push_back(0);
            }
        }
        
        return sequence;
    }
};

// 测试DHT11初始化
TEST_F(DHT11Test, Initialization) {
    using ::testing::_;
    using ::testing::Return;
    
    MockGPIO gpio;
    
    // 期望在初始化时不会调用任何GPIO方法
    EXPECT_CALL(gpio, configGPIO(_, _)).Times(0);
    EXPECT_CALL(gpio, writeGPIO(_, _)).Times(0);
    EXPECT_CALL(gpio, readGPIO(_)).Times(0);
    
    DHT11 dht(gpio);
    
    // 验证初始化并启动/停止不会出错
    ASSERT_NO_THROW(dht.start());
    ASSERT_NO_THROW(dht.stop());
}

// 测试DHT11回调注册
TEST_F(DHT11Test, CallbackRegistration) {
    MockGPIO gpio;
    DHT11 dht(gpio);
    
    bool callbackCalled = false;
    DHTReading callbackData = {0};
    
    // 注册回调
    dht.registerCallback([&callbackCalled, &callbackData](const DHTReading& reading) {
        callbackCalled = true;
        callbackData = reading;
    });
    
    // 注册回调后不应该自动调用
    EXPECT_FALSE(callbackCalled);
}

// 测试DHT11数据读取成功
TEST_F(DHT11Test, ReadDataSuccess) {
    TestableGPIO gpio;
    DHT11 dht(gpio);
    
    // 创建模拟数据：湿度=42.5%, 温度=25.3°C
    uint8_t mockData[5] = {42, 5, 25, 3, 42+5+25+3};  // 最后一位是校验和
    
    // 设置DHT11响应序列
    gpio.setReadSequence(generateDHT11DataSequence(mockData));
    
    // 读取数据
    DHTReading reading;
    bool success = dht.readData(reading);
    
    // 验证结果
    ASSERT_TRUE(success) << "数据读取应该成功";
    EXPECT_FLOAT_EQ(reading.humidity, 42.5f) << "湿度应为42.5%";
    EXPECT_FLOAT_EQ(reading.temp_celsius, 25.3f) << "温度应为25.3°C";
}

// 测试DHT11数据读取失败 - 校验和错误
TEST_F(DHT11Test, ReadDataChecksumError) {
    TestableGPIO gpio;
    DHT11 dht(gpio);
    
    // 创建校验和错误的模拟数据
    uint8_t mockData[5] = {42, 5, 25, 3, 99};  // 错误的校验和
    
    // 设置DHT11响应序列
    gpio.setReadSequence(generateDHT11DataSequence(mockData));
    
    // 读取数据
    DHTReading reading;
    bool success = dht.readData(reading);
    
    // 验证结果
    ASSERT_FALSE(success) << "校验和错误应导致读取失败";
}

// 测试DHT11数据读取失败 - 无响应
TEST_F(DHT11Test, ReadDataNoResponse) {
    using ::testing::Return;
    
    MockGPIO gpio;
    
    // 配置GPIO操作预期
    EXPECT_CALL(gpio, configGPIO(GPIOdef::DHT_IO, GPIOconfig::OUTPUT)).Times(::testing::AtLeast(1)).WillRepeatedly(Return(true));
    EXPECT_CALL(gpio, writeGPIO(GPIOdef::DHT_IO, 0)).Times(::testing::AtLeast(1)).WillRepeatedly(Return(true));
    EXPECT_CALL(gpio, writeGPIO(GPIOdef::DHT_IO, 1)).Times(::testing::AtLeast(1)).WillRepeatedly(Return(true));
    EXPECT_CALL(gpio, configGPIO(GPIOdef::DHT_IO, GPIOconfig::INPUT)).Times(::testing::AtLeast(1)).WillRepeatedly(Return(true));
    
    // 模拟传感器无响应 - 始终返回高电平
    EXPECT_CALL(gpio, readGPIO(GPIOdef::DHT_IO)).WillRepeatedly(Return(1));
    
    DHT11 dht(gpio);
    
    // 读取数据
    DHTReading reading;
    bool success = dht.readData(reading);
    
    // 验证结果
    ASSERT_FALSE(success) << "传感器无响应应导致读取失败";
}

#ifdef TEST_SMOOTHING_ENABLED
// 测试数据平滑功能 - 需要在dht.h中添加testSmoothReadings方法
TEST_F(DHT11Test, DataSmoothing) {
    MockGPIO gpio;
    DHT11 dht(gpio);
    
    // 创建测试数据
    std::array<DHTReading, 5> readings = {{
        {60.0f, 20.0f},
        {62.0f, 21.0f},
        {58.0f, 19.0f},
        {61.0f, 20.5f},
        {59.0f, 19.5f}
    }};
    
    // 依次处理每个读数
    for (int i = 0; i < 5; i++) {
        DHTReading reading = readings[i];
        dht.testSmoothReadings(reading);
        
        // 最后一个读数应该进行了平滑处理
        if (i == 4) {
            // 计算期望的平均值
            float expectedHumidity = 0.0f;
            float expectedTemp = 0.0f;
            
            for (int j = 0; j < 5; j++) {
                expectedHumidity += readings[j].humidity;
                expectedTemp += readings[j].temp_celsius;
            }
            
            expectedHumidity /= 5.0f;
            expectedTemp /= 5.0f;
            
            // 验证平滑后的结果
            EXPECT_NEAR(reading.humidity, expectedHumidity, 0.001f);
            EXPECT_NEAR(reading.temp_celsius, expectedTemp, 0.001f);
        }
    }
}
#endif

// 真实硬件测试 - 默认禁用
#ifdef HARDWARE_TEST_ENABLED
TEST_F(DHT11Test, DISABLED_RealHardwareTest) {
    // 创建真实的GPIO实例
    GPIO gpio;
    gpio.gpio_init();  // 初始化GPIO
    
    DHT11 dht(gpio);
    
    // 读取DHT11数据
    DHTReading reading;
    bool success = dht.readData(reading);
    
    if (success) {
        std::cout << "从DHT11传感器读取数据成功:" << std::endl;
        std::cout << "  湿度: " << reading.humidity << "%" << std::endl;
        std::cout << "  温度: " << reading.temp_celsius << "°C" << std::endl;
        
        // 基本验证
        EXPECT_GE(reading.humidity, 0.0f);
        EXPECT_LE(reading.humidity, 100.0f);
        EXPECT_GE(reading.temp_celsius, -10.0f);
        EXPECT_LE(reading.temp_celsius, 50.0f);
    } else {
        GTEST_SKIP() << "无法读取DHT11传感器，跳过测试";
    }
}
#endif

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
