#include "i2c_display.h"
#include <iostream>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>
#include <cstring>
#include <cerrno>
#include <cstdlib>

// I2C bus path and SSD1306 device address (adjust according to actual hardware)
static const char* I2C_BUS = "/dev/i2c-1";
static const int I2C_ADDR = 0x3C;

// Complete 5x7 font array (ASCII 32~126, total 95 characters)
static const uint8_t font5x7[95][5] = {
  {0x00,0x00,0x00,0x00,0x00}, // ' ' (32)
  {0x00,0x00,0x5F,0x00,0x00}, // '!'
  {0x00,0x07,0x00,0x07,0x00}, // '"'
  {0x14,0x7F,0x14,0x7F,0x14}, // '#'
  {0x24,0x2A,0x7F,0x2A,0x12}, // '$'
  {0x23,0x13,0x08,0x64,0x62}, // '%'
  {0x36,0x49,0x55,0x22,0x50}, // '&'
  {0x00,0x05,0x03,0x00,0x00}, // '''
  {0x00,0x1C,0x22,0x41,0x00}, // '('
  {0x00,0x41,0x22,0x1C,0x00}, // ')'
  {0x14,0x08,0x3E,0x08,0x14}, // '*'
  {0x08,0x08,0x3E,0x08,0x08}, // '+'
  {0x00,0x50,0x30,0x00,0x00}, // ','
  {0x08,0x08,0x08,0x08,0x08}, // '-'
  {0x00,0x60,0x60,0x00,0x00}, // '.'
  {0x20,0x10,0x08,0x04,0x02}, // '/'
  {0x3E,0x51,0x49,0x45,0x3E}, // '0'
  {0x00,0x42,0x7F,0x40,0x00}, // '1'
  {0x42,0x61,0x51,0x49,0x46}, // '2'
  {0x21,0x41,0x45,0x4B,0x31}, // '3'
  {0x18,0x14,0x12,0x7F,0x10}, // '4'
  {0x27,0x45,0x45,0x45,0x39}, // '5'
  {0x3C,0x4A,0x49,0x49,0x30}, // '6'
  {0x01,0x71,0x09,0x05,0x03}, // '7'
  {0x36,0x49,0x49,0x49,0x36}, // '8'
  {0x06,0x49,0x49,0x29,0x1E}, // '9'
  {0x00,0x36,0x36,0x00,0x00}, // ':'
  {0x00,0x56,0x36,0x00,0x00}, // ';'
  {0x08,0x14,0x22,0x41,0x00}, // '<'
  {0x14,0x14,0x14,0x14,0x14}, // '='
  {0x00,0x41,0x22,0x14,0x08}, // '>'
  {0x02,0x01,0x51,0x09,0x06}, // '?'
  {0x32,0x49,0x79,0x41,0x3E}, // '@'
  {0x7E,0x11,0x11,0x11,0x7E}, // 'A'
  {0x7F,0x49,0x49,0x49,0x36}, // 'B'
  {0x3E,0x41,0x41,0x41,0x22}, // 'C'
  {0x7F,0x41,0x41,0x22,0x1C}, // 'D'
  {0x7F,0x49,0x49,0x49,0x41}, // 'E'
  {0x7F,0x09,0x09,0x09,0x01}, // 'F'
  {0x3E,0x41,0x49,0x49,0x7A}, // 'G'
  {0x7F,0x08,0x08,0x08,0x7F}, // 'H'
  {0x00,0x41,0x7F,0x41,0x00}, // 'I'
  {0x20,0x40,0x41,0x3F,0x01}, // 'J'
  {0x7F,0x08,0x14,0x22,0x41}, // 'K'
  {0x7F,0x40,0x40,0x40,0x40}, // 'L'
  {0x7F,0x02,0x04,0x02,0x7F}, // 'M'
  {0x7F,0x04,0x08,0x10,0x7F}, // 'N'
  {0x3E,0x41,0x41,0x41,0x3E}, // 'O'
  {0x7F,0x09,0x09,0x09,0x06}, // 'P'
  {0x3E,0x41,0x51,0x21,0x5E}, // 'Q'
  {0x7F,0x09,0x19,0x29,0x46}, // 'R'
  {0x46,0x49,0x49,0x49,0x31}, // 'S'
  {0x01,0x01,0x7F,0x01,0x01}, // 'T'
  {0x3F,0x40,0x40,0x40,0x3F}, // 'U'
  {0x1F,0x20,0x40,0x20,0x1F}, // 'V'
  {0x3F,0x40,0x38,0x40,0x3F}, // 'W'
  {0x63,0x14,0x08,0x14,0x63}, // 'X'
  {0x07,0x08,0x70,0x08,0x07}, // 'Y'
  {0x61,0x51,0x49,0x45,0x43}, // 'Z'
  {0x00,0x7F,0x41,0x41,0x00}, // '['
  {0x02,0x04,0x08,0x10,0x20}, // '\'
  {0x00,0x41,0x41,0x7F,0x00}, // ']'
  {0x04,0x02,0x01,0x02,0x04}, // '^'
  {0x40,0x40,0x40,0x40,0x40}, // '_'
  {0x00,0x01,0x02,0x04,0x00}, // '`'
  {0x20,0x54,0x54,0x54,0x78}, // 'a'
  {0x7F,0x48,0x44,0x44,0x38}, // 'b'
  {0x38,0x44,0x44,0x44,0x20}, // 'c'
  {0x38,0x44,0x44,0x48,0x7F}, // 'd'
  {0x38,0x54,0x54,0x54,0x18}, // 'e'
  {0x08,0x7E,0x09,0x01,0x02}, // 'f'
  {0x0C,0x52,0x52,0x52,0x3E}, // 'g'
  {0x7F,0x08,0x04,0x04,0x78}, // 'h'
  {0x00,0x44,0x7D,0x40,0x00}, // 'i'
  {0x20,0x40,0x44,0x3D,0x00}, // 'j'
  {0x7F,0x10,0x28,0x44,0x00}, // 'k'
  {0x00,0x41,0x7F,0x40,0x00}, // 'l'
  {0x7C,0x04,0x18,0x04,0x78}, // 'm'
  {0x7C,0x08,0x04,0x04,0x78}, // 'n'
  {0x38,0x44,0x44,0x44,0x38}, // 'o'
  {0x7C,0x14,0x14,0x14,0x08}, // 'p'
  {0x08,0x14,0x14,0x18,0x7C}, // 'q'
  {0x7C,0x08,0x04,0x04,0x08}, // 'r'
  {0x48,0x54,0x54,0x54,0x20}, // 's'
  {0x04,0x3F,0x44,0x40,0x20}, // 't'
  {0x3C,0x40,0x40,0x20,0x7C}, // 'u'
  {0x1C,0x20,0x40,0x20,0x1C}, // 'v'
  {0x3C,0x40,0x30,0x40,0x3C}, // 'w'
  {0x44,0x28,0x10,0x28,0x44}, // 'x'
  {0x0C,0x50,0x50,0x50,0x3C}, // 'y'
  {0x44,0x64,0x54,0x4C,0x44}, // 'z'
  {0x00,0x08,0x36,0x41,0x00}, // '{'
  {0x00,0x00,0x7F,0x00,0x00}, // '|'
  {0x00,0x41,0x36,0x08,0x00}, // '}'
  {0x10,0x08,0x08,0x10,0x08}  // '~'
};

// Singleton implementation
I2cDisplay& I2cDisplay::getInstance() {
    static I2cDisplay instance;
    return instance;
}

I2cDisplay::I2cDisplay() : i2c_fd(-1) {
    clearBuffer();
}

I2cDisplay::~I2cDisplay() {
    if (i2c_fd >= 0) {
        close(i2c_fd);
    }
}

// Initialize SSD1306
void I2cDisplay::init() {
    i2c_fd = open(I2C_BUS, O_RDWR);
    if (i2c_fd < 0) {
        std::cerr << "Failed to open I2C bus: " << strerror(errno) << std::endl;
        exit(EXIT_FAILURE);
    }
    if (ioctl(i2c_fd, I2C_SLAVE, I2C_ADDR) < 0) {
        std::cerr << "Failed to set I2C address: " << strerror(errno) << std::endl;
        close(i2c_fd);
        i2c_fd = -1;
        exit(EXIT_FAILURE);
    }
    // Send SSD1306 initialization command sequence
    sendCommand(0xAE); // Turn off display
    sendCommand(0x20); // Set memory addressing mode
    sendCommand(0x00); // Horizontal addressing mode
    sendCommand(0x40); // Set start line address to 0
    sendCommand(0x8D); // Enable charge pump command
    sendCommand(0x14); // Enable charge pump
    sendCommand(0xA1); // Segment remap
    sendCommand(0xC8); // COM output scan direction
    sendCommand(0xA8); // Multiplex ratio
    sendCommand(0x3F); // 64 lines
    sendCommand(0xD3); // Display offset
    sendCommand(0x00);
    sendCommand(0xD5); // Clock divide ratio/oscillator frequency
    sendCommand(0x80);
    sendCommand(0xD9); // Pre-charge period
    sendCommand(0xF1);
    sendCommand(0xDA); // COM pins hardware configuration
    sendCommand(0x12);
    sendCommand(0xDB); // VCOMH deselect level
    sendCommand(0x40);
    sendCommand(0x81); // Contrast control
    sendCommand(0x7F);
    sendCommand(0xA4); // Output follows RAM content
    sendCommand(0xA6); // Normal display
    sendCommand(0xAF); // Turn on display
    std::cout << "SSD1306 initialization complete" << std::endl;
}

// Send command
void I2cDisplay::sendCommand(uint8_t cmd) {
    uint8_t data[2] = {0x00, cmd};
    ssize_t bytes = write(i2c_fd, data, 2);
    if (bytes != 2) {
        std::cerr << "Failed to send command 0x" << std::hex << (int)cmd << " failed: " << strerror(errno) << std::endl;
    }
    usleep(1000);
}

// Send buffer data
void I2cDisplay::sendBuffer(const uint8_t* buf, size_t len) {
    uint8_t* tmp = new uint8_t[len + 1];
    tmp[0] = 0x40; // Control byte, indicating data
    memcpy(tmp + 1, buf, len);
    ssize_t bytes = write(i2c_fd, tmp, len + 1);
    if (bytes != static_cast<ssize_t>(len + 1)) {
        std::cerr << "Sending display data failed: " << strerror(errno) << std::endl;
    }
    delete[] tmp;
}

// Clear buffer
void I2cDisplay::clearBuffer() {
    memset(buffer, 0, sizeof(buffer));
}

// Calculate text width
int I2cDisplay::textWidth(const std::string &text) {
    return text.size() * 6;
}

// Draw a single character on page 0
void I2cDisplay::drawChar(int x, char c) {
    if (c < 32 || c > 126) c = 32;
    int index = c - 32;
    for (int col = 0; col < 5; col++) {
        int pos = x + col;
        if (pos >= 0 && pos < 128) {
            buffer[pos] = font5x7[index][col];
        }
    }
    int pos = x + 5;
    if (pos >= 0 && pos < 128) {
        buffer[pos] = 0x00;
    }
}

// Display single-line text (default on page 0)
void I2cDisplay::displayText(const std::string &text) {
    clearBuffer();
    int w = textWidth(text);
    int start_x = (128 - w) / 2;
    int x = start_x;
    for (char c : text) {
        drawChar(x, c);
        x += 6;
    }
    sendBuffer(buffer, sizeof(buffer));
}

// Draw a single character on a specified page
void I2cDisplay::drawCharAt(int x, int page, char c) {
    if (c < 32 || c > 126) c = 32;
    int index = c - 32;
    int offset = page * 128; // 128 bytes per page
    for (int col = 0; col < 5; col++) {
        int pos = offset + x + col;
        if ((x + col) < 128) {
            buffer[pos] = font5x7[index][col];
        }
    }
    int pos = offset + x + 5;
    if ((x + 5) < 128) {
        buffer[pos] = 0x00;
    }
}

// Draw text on a specified page, auto-centered
void I2cDisplay::displayTextAt(int page, const std::string &text) {
    int w = text.size() * 6;
    int start_x = (128 - w) / 2;
    int x = start_x;
    for (char c : text) {
        drawCharAt(x, page, c);
        x += 6;
    }
}

// Display two lines of text (e.g., temperature and humidity data) on page 2 and page 4
void I2cDisplay::displayMultiLine(const std::string &line1, const std::string &line2) {
    clearBuffer();
    displayTextAt(2, line1);
    displayTextAt(4, line2);
    sendBuffer(buffer, sizeof(buffer));
}


// Display "INVASION" changed to display two lines of text:
// Page 0: Display "INVASION"
// Page 2: Display "Enter password"
// Page 4: Display "to unlock"
void I2cDisplay::displayIntrusion() {
    clearBuffer();
    displayTextAt(0, "INVASION");
    displayTextAt(2, "Enter password");
    displayTextAt(4, "to unlock");
    sendBuffer(buffer, sizeof(buffer));
    std::cout << "[DISPLAY] Intrusion detected" << std::endl;
    if (eventCallback) eventCallback("INVASION");
}


// Display SAFE and temperature/humidity data: Page 0 shows "SAFE", Page 2 shows temperature, Page 4 shows humidity
void I2cDisplay::displaySafeAndDHT(const std::string &tempStr, const std::string &humStr) {
    clearBuffer();
    displayTextAt(0, "SAFE");
    displayTextAt(2, tempStr);
    displayTextAt(4, humStr);
    sendBuffer(buffer, sizeof(buffer));
}

void I2cDisplay::displayWrongPassword() {
    clearBuffer();
    // Assume that the first line is displayed on page 0, and the second line on page 2
    displayTextAt(0, "Wrong password,");
    displayTextAt(2, "please try again");
    sendBuffer(buffer, sizeof(buffer));
    std::cout << "[DISPLAY] Incorrect password, please try again." << std::endl;
}

void I2cDisplay::displayPasswordStars(const std::string &stars) {
    clearBuffer();
    // Assume we display password feedback on page 6
    displayTextAt(6, stars);
    sendBuffer(buffer, sizeof(buffer));
    std::cout << "[DISPLAY] Entered password: " << stars << std::endl;
}

void I2cDisplay::registerEventCallback(std::function<void(const std::string&)> callback) {
    eventCallback = callback;
}


