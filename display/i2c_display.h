#ifndef I2C_DISPLAY_H
#define I2C_DISPLAY_H

#include <string>
#include <functional>
#include <cstdint>

class I2cDisplay {
public:
    // Get singleton instance
    static I2cDisplay& getInstance();

    // Initialize SSD1306 (open I2C bus and send initialization commands)
    void init();

    // Display single-line text (default on page 0, centered)
    void displayText(const std::string &text);

    // Draw text on the specified page, automatically centered horizontally
    void displayTextAt(int page, const std::string &text);

    // Display two lines of text (e.g., temperature and humidity data) on specified pages (e.g., page 2 and page 4)
    void displayMultiLine(const std::string &line1, const std::string &line2);

    // Display "INVASION" (centered, covers the entire screen)
    void displayIntrusion();

    // Display "SAFE" (only show SAFE, without updating temperature and humidity)
    void displaySafe();

    // In SAFE state, simultaneously display "SAFE" and temperature/humidity data,
    // Display "SAFE" on page 0, temperature on page 2, humidity on page 4
    void displaySafeAndDHT(const std::string &tempStr, const std::string &humStr);

    // Display error message: e.g., 'Wrong password, please try again.'
    void displayWrongPassword();

    // Display password input feedback (a string of '*'), e.g., on page 6
    void displayPasswordStars(const std::string &stars);

    // Register event callback (optional, for debugging or interaction)
    void registerEventCallback(std::function<void(const std::string&)> callback);

private:
    I2cDisplay();
    ~I2cDisplay();
    I2cDisplay(const I2cDisplay&) = delete;
    I2cDisplay& operator=(const I2cDisplay&) = delete;

    // Send a single command to SSD1306 (control byte 0x00)
    void sendCommand(uint8_t cmd);
    // Send the entire display buffer to SSD1306 (control byte 0x40)
    void sendBuffer(const uint8_t* buf, size_t len);

    // Clear local display buffer
    void clearBuffer();

    // Helper function: calculate text width (each character 5 pixels + 1 pixel spacing)
    int textWidth(const std::string &text);
    
    // Helper function: draw a single character on page 0 (5x7 font)
    void drawChar(int x, char c);
    
    // New: draw a single character on the specified page (page range 0-7, each page 8 pixels)
    void drawCharAt(int x, int page, char c);

private:
    int i2c_fd; // I2C file descriptor
    std::function<void(const std::string&)> eventCallback;
    uint8_t buffer[1024]; // Display buffer: 128 x 64 / 8 = 1024 bytes
};

#endif // I2C_DISPLAY_H
