#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <chrono>
#include <thread>
#include <vector>
#include <iostream>
#include "../dht/dht.h"
#include "../gpio/gpio.h"

// Mock GPIO class that inherits from GPIO but overrides hardware access methods
class MockGPIO : public GPIO {
public:
    // Use GMock macros to mock methods
    MOCK_METHOD(bool, configGPIO, (int pin_number, int config_num), (override));
    MOCK_METHOD(bool, writeGPIO, (int pin_number, int value), (override));
    MOCK_METHOD(int, readGPIO, (int pin_number), (override));
    
    // Override constructor to prevent hardware access
    MockGPIO() {
        // No hardware initialization
    }
    
    // Override other methods that might access hardware
    void gpio_init() override {}
    void start() override {}
    void stop() override {}
    void registerCallback(int pin_number, GPIO::GPIOEventCallbackInterface* callback) override {}
    
    // Override destructor
    ~MockGPIO() override {}
};

// Controllable GPIO test class for simulating sensor behavior
class TestableGPIO : public GPIO {
public:
    TestableGPIO() {
        useSequence = false;
        
        // Initialize pin state arrays
        for (int i = 0; i < 64; i++) {
            pinValues[i] = 0;
            pinDirections[i] = GPIOconfig::INPUT;
        }
    }
    
    bool configGPIO(int pin_number, int config_num) override {
        if (pin_number < 64) {
            pinDirections[pin_number] = config_num;
            return true;
        }
        return false;
    }
    
    bool writeGPIO(int pin_number, int value) override {
        if (pin_number < 64) {
            pinValues[pin_number] = value;
            return true;
        }
        return false;
    }
    
    int readGPIO(int pin_number) override {
        // If using preset sequence for DHT pin, return sequence value
        if (useSequence && pin_number == GPIOdef::DHT_IO && sequenceIndex < readSequence.size()) {
            return readSequence[sequenceIndex++];
        }
        
        if (pin_number < 64) {
            return pinValues[pin_number];
        }
        return -1; // Error state
    }
    
    // Set read sequence for simulating DHT11 communication
    void setReadSequence(const std::vector<int>& sequence) {
        readSequence = sequence;
        sequenceIndex = 0;
        useSequence = true;
    }
    
    void resetSequence() {
        sequenceIndex = 0;
    }
    
    void disableSequence() {
        useSequence = false;
    }
    
    void gpio_init() override {}
    void start() override {}
    void stop() override {}
    void registerCallback(int pin_number, GPIO::GPIOEventCallbackInterface* callback) override {}
    
    ~TestableGPIO() override {}
    
private:
    int pinValues[64];  // Store pin values
    int pinDirections[64];  // Store pin directions
    std::vector<int> readSequence;  // Preset read sequence
    size_t sequenceIndex = 0;  // Current sequence index
    bool useSequence;  // Whether to use sequence
};

// Test fixture class for DHT11 tests
class DHT11Test : public ::testing::Test {
protected:
    void SetUp() override {}
    
    void TearDown() override {}
    
    // Generate DHT11 response sequence
    std::vector<int> generateDHT11ResponseSequence() {
        std::vector<int> sequence;
        
        // Initial state (high level)
        for (int i = 0; i < 5; i++) sequence.push_back(1);
        
        // DHT11 response signal (first low then high)
        for (int i = 0; i < 10; i++) sequence.push_back(0);  // Pull low ~80us
        for (int i = 0; i < 10; i++) sequence.push_back(1);  // Pull high ~80us
        
        return sequence;
    }
    
    // Generate a complete data reading sequence with specified bytes
    std::vector<int> generateDHT11DataSequence(const uint8_t data[5]) {
        std::vector<int> sequence = generateDHT11ResponseSequence();
        
        // Add data bits (5 bytes, 8 bits per byte)
        for (int byte = 0; byte < 5; byte++) {
            for (int bit = 7; bit >= 0; bit--) {
                // Bit start signal (low level ~50us)
                for (int i = 0; i < 5; i++) sequence.push_back(0);
                
                // Data bit value (high level duration determines 0 or 1)
                bool isOne = (data[byte] >> bit) & 0x01;
                int highDuration = isOne ? 12 : 5;  // 1~70us, 0~28us
                for (int i = 0; i < highDuration; i++) sequence.push_back(1);
                
                sequence.push_back(0);
            }
        }
        
        return sequence;
    }
};

// Test DHT11 initialization
TEST_F(DHT11Test, Initialization) {
    using ::testing::_;
    using ::testing::Return;
    
    MockGPIO gpio;
    
    // Expect no GPIO methods will be called during initialization
    EXPECT_CALL(gpio, configGPIO(_, _)).Times(0);
    EXPECT_CALL(gpio, writeGPIO(_, _)).Times(0);
    EXPECT_CALL(gpio, readGPIO(_)).Times(0);
    
    DHT11 dht(gpio);
    
    // Verify initialization and start/stop don't cause errors
    ASSERT_NO_THROW(dht.start());
    ASSERT_NO_THROW(dht.stop());
}

// Test DHT11 callback registration
TEST_F(DHT11Test, CallbackRegistration) {
    MockGPIO gpio;
    DHT11 dht(gpio);
    
    bool callbackCalled = false;
    DHTReading callbackData = {0};
    
    // Register callback
    dht.registerCallback([&callbackCalled, &callbackData](const DHTReading& reading) {
        callbackCalled = true;
        callbackData = reading;
    });
    
    // Callback should not be automatically called
    EXPECT_FALSE(callbackCalled);
}

// Test DHT11 data reading success
TEST_F(DHT11Test, ReadDataSuccess) {
    TestableGPIO gpio;
    DHT11 dht(gpio);
    
    // Create mock data: humidity=42.5%, temperature=25.3°C
    uint8_t mockData[5] = {42, 5, 25, 3, 42+5+25+3};  // Last byte is checksum
    
    gpio.setReadSequence(generateDHT11DataSequence(mockData));
    
    DHTReading reading;
    bool success = dht.readData(reading);
    
    // Verify results
    ASSERT_TRUE(success) << "Data reading should succeed";
    EXPECT_FLOAT_EQ(reading.humidity, 42.5f) << "Humidity should be 42.5%";
    EXPECT_FLOAT_EQ(reading.temp_celsius, 25.3f) << "Temperature should be 25.3°C";
}

// Test DHT11 data reading failure - checksum error
TEST_F(DHT11Test, ReadDataChecksumError) {
    TestableGPIO gpio;
    DHT11 dht(gpio);
    
    // Create mock data with checksum error
    uint8_t mockData[5] = {42, 5, 25, 3, 99};  // Wrong checksum
    
    gpio.setReadSequence(generateDHT11DataSequence(mockData));
    
    DHTReading reading;
    bool success = dht.readData(reading);
    
    ASSERT_FALSE(success) << "Checksum error should cause reading failure";
}

// Test DHT11 data reading failure - no response
TEST_F(DHT11Test, ReadDataNoResponse) {
    using ::testing::Return;
    
    MockGPIO gpio;
    
    // Configure expected GPIO operations
    EXPECT_CALL(gpio, configGPIO(GPIOdef::DHT_IO, GPIOconfig::OUTPUT)).Times(::testing::AtLeast(1)).WillRepeatedly(Return(true));
    EXPECT_CALL(gpio, writeGPIO(GPIOdef::DHT_IO, 0)).Times(::testing::AtLeast(1)).WillRepeatedly(Return(true));
    EXPECT_CALL(gpio, writeGPIO(GPIOdef::DHT_IO, 1)).Times(::testing::AtLeast(1)).WillRepeatedly(Return(true));
    EXPECT_CALL(gpio, configGPIO(GPIOdef::DHT_IO, GPIOconfig::INPUT)).Times(::testing::AtLeast(1)).WillRepeatedly(Return(true));
    
    // Simulate sensor non-response - always return high level
    EXPECT_CALL(gpio, readGPIO(GPIOdef::DHT_IO)).WillRepeatedly(Return(1));
    
    DHT11 dht(gpio);
    
    DHTReading reading;
    bool success = dht.readData(reading);
    
    ASSERT_FALSE(success) << "Sensor non-response should cause reading failure";
}

#ifdef TEST_SMOOTHING_ENABLED
// Test data smoothing functionality
TEST_F(DHT11Test, DataSmoothing) {
    MockGPIO gpio;
    DHT11 dht(gpio);
    
    // Create test data
    std::array<DHTReading, 5> readings = {{
        {60.0f, 20.0f},
        {62.0f, 21.0f},
        {58.0f, 19.0f},
        {61.0f, 20.5f},
        {59.0f, 19.5f}
    }};
    
    // Process each reading in sequence
    for (int i = 0; i < 5; i++) {
        DHTReading reading = readings[i];
        dht.testSmoothReadings(reading);
        
        // The last reading should have been smoothed
        if (i == 4) {
            // Calculate expected averages
            float expectedHumidity = 0.0f;
            float expectedTemp = 0.0f;
            
            for (int j = 0; j < 5; j++) {
                expectedHumidity += readings[j].humidity;
                expectedTemp += readings[j].temp_celsius;
            }
            
            expectedHumidity /= 5.0f;
            expectedTemp /= 5.0f;
            
            // Verify smoothed results
            EXPECT_NEAR(reading.humidity, expectedHumidity, 0.001f);
            EXPECT_NEAR(reading.temp_celsius, expectedTemp, 0.001f);
        }
    }
}
#endif

// Real hardware test - disabled by default
#ifdef HARDWARE_TEST_ENABLED
TEST_F(DHT11Test, DISABLED_RealHardwareTest) {
    // Create real GPIO instance
    GPIO gpio;
    gpio.gpio_init();  // Initialize GPIO
    
    DHT11 dht(gpio);
    
    // Read DHT11 data
    DHTReading reading;
    bool success = dht.readData(reading);
    
    if (success) {
        std::cout << "Successfully read data from DHT11 sensor:" << std::endl;
        std::cout << "  Humidity: " << reading.humidity << "%" << std::endl;
        std::cout << "  Temperature: " << reading.temp_celsius << "°C" << std::endl;
        
        // Basic validation
        EXPECT_GE(reading.humidity, 0.0f);
        EXPECT_LE(reading.humidity, 100.0f);
        EXPECT_GE(reading.temp_celsius, -10.0f);
        EXPECT_LE(reading.temp_celsius, 50.0f);
    } else {
        GTEST_SKIP() << "Cannot read DHT11 sensor, skipping test";
    }
}
#endif

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}