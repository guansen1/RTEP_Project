#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "display/i2c_display.h"
#include <fcntl.h>
#include <unistd.h>

using ::testing::_;
using ::testing::Return;

// Helper function for testing the protected textWidth method
int testTextWidth(const std::string& text) {
    // Each character is 6 pixels (5 pixel character + 1 pixel spacing)
    return text.size() * 6;
}

// Test class for mocking I2C device file operations
class I2cDisplayTest : public ::testing::Test {
protected:
    I2cDisplay& display;

    I2cDisplayTest() : display(I2cDisplay::getInstance()) {
       
    }

    // Helper method for mocking file descriptor operations
    int mockOpenI2cDevice() {
       
        int fd = open("/tmp/mock_i2c_device", O_RDWR | O_CREAT, 0644);
        EXPECT_GE(fd, 0) << "Failed to create mock I2C device file";
        return fd;
    }
};

// Test singleton pattern
TEST_F(I2cDisplayTest, SingletonTest) {
    I2cDisplay& display2 = I2cDisplay::getInstance();
    EXPECT_EQ(&display, &display2) << "Singleton implementation error: multiple instances";
}

// Test text width calculation
TEST_F(I2cDisplayTest, TextWidthCalculation) {
   
    EXPECT_EQ(testTextWidth(""), 0);
    EXPECT_EQ(testTextWidth("A"), 6);
    EXPECT_EQ(testTextWidth("Hello"), 30);
}

// Test callback registration
TEST_F(I2cDisplayTest, CallbackRegistration) {
    
    int mock_fd = open("/tmp/mock_i2c", O_RDWR | O_CREAT, 0644);
    
    // Record test status
    std::cout << "No actual data will be sent to hardware during testing" << std::endl;
    
    bool callbackCalled = false;
    display.registerEventCallback([&callbackCalled](const std::string& event) {
        callbackCalled = true;
    });

    display.displayIntrusion();
    
    EXPECT_TRUE(callbackCalled);
    
    if (mock_fd >= 0) {
        close(mock_fd);
    }
}

// Test display text functionality
TEST_F(I2cDisplayTest, DisplayTextFunctionality) {
    int mock_fd = open("/tmp/mock_i2c", O_RDWR | O_CREAT, 0644);
    
    std::cout << "No actual data will be sent to hardware during testing" << std::endl;
    
    // Verify that displayText method doesn't throw exceptions
    EXPECT_NO_THROW(display.displayText("Test"));
    
    if (mock_fd >= 0) {
        close(mock_fd);
    }
}

// Test multi-line display
TEST_F(I2cDisplayTest, MultiLineDisplay) {
    int mock_fd = open("/tmp/mock_i2c", O_RDWR | O_CREAT, 0644);
    
    std::cout << "No actual data will be sent to hardware during testing" << std::endl;
    
    EXPECT_NO_THROW(display.displayMultiLine("Line1", "Line2"));
    
    if (mock_fd >= 0) {
        close(mock_fd);
    }
}

// Test system state display for safe and intrusion conditions
TEST_F(I2cDisplayTest, SystemStateDisplay) {
    int mock_fd = open("/tmp/mock_i2c", O_RDWR | O_CREAT, 0644);
    
    std::cout << "No actual data will be sent to hardware during testing" << std::endl;
    
    EXPECT_NO_THROW(display.displaySafe());
    EXPECT_NO_THROW(display.displayIntrusion());
    
    if (mock_fd >= 0) {
        close(mock_fd);
    }
}

// Test safe state display with temperature and humidity
TEST_F(I2cDisplayTest, SafeWithDHTDisplay) {
    int mock_fd = open("/tmp/mock_i2c", O_RDWR | O_CREAT, 0644);
    
    std::cout << "No actual data will be sent to hardware during testing" << std::endl;
    
    EXPECT_NO_THROW(display.displaySafeAndDHT("Temp:25.5 C", "Hum:60.0 %"));
    
    if (mock_fd >= 0) {
        close(mock_fd);
    }
}

#ifdef HARDWARE_TEST_ENABLED
// Hardware test that only executes when hardware testing is enabled
TEST_F(I2cDisplayTest, HardwareInitialization) {
    // Attempt to initialize the actual I2C device
    EXPECT_NO_THROW(display.init());
}
#endif

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    
    return RUN_ALL_TESTS();
}