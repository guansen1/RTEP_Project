#include "i2c_display.h"
#include <iostream>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>
#include <cstring>
#include <cerrno>
#include <cstdlib>

// I2C 总线路径与 SSD1306 设备地址（请根据实际硬件调整）
static const char* I2C_BUS = "/dev/i2c-1";
static const int I2C_ADDR = 0x3C;

// 完整的 5x7 字体数组（ASCII 32~126，共 95 个字符）
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

// 单例实现
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

// 初始化 SSD1306
void I2cDisplay::init() {
    i2c_fd = open(I2C_BUS, O_RDWR);
    if (i2c_fd < 0) {
        std::cerr << "无法打开 I2C 总线: " << strerror(errno) << std::endl;
        exit(EXIT_FAILURE);
    }
    if (ioctl(i2c_fd, I2C_SLAVE, I2C_ADDR) < 0) {
        std::cerr << "无法设置 I2C 地址: " << strerror(errno) << std::endl;
        close(i2c_fd);
        i2c_fd = -1;
        exit(EXIT_FAILURE);
    }
    // 发送 SSD1306 初始化命令序列
    sendCommand(0xAE); // 关闭显示
    sendCommand(0x20); // 设置内存寻址模式
    sendCommand(0x00); // 水平寻址模式
    sendCommand(0x40); // 起始行地址为0
    sendCommand(0x8D); // 使能充电泵命令
    sendCommand(0x14); // 使能充电泵
    sendCommand(0xA1); // 段重映射
    sendCommand(0xC8); // COM 输出扫描方向
    sendCommand(0xA8); // 多路复用率
    sendCommand(0x3F); // 64 行
    sendCommand(0xD3); // 显示偏移
    sendCommand(0x00);
    sendCommand(0xD5); // 时钟分频/振荡器频率
    sendCommand(0x80);
    sendCommand(0xD9); // 预充电周期
    sendCommand(0xF1);
    sendCommand(0xDA); // COM 引脚硬件配置
    sendCommand(0x12);
    sendCommand(0xDB); // VCOMH 去激电平
    sendCommand(0x40);
    sendCommand(0x81); // 对比度控制
    sendCommand(0x7F);
    sendCommand(0xA4); // 输出跟随RAM
    sendCommand(0xA6); // 正常显示
    sendCommand(0xAF); // 打开显示
    std::cout << "SSD1306 初始化完成" << std::endl;
}

// 发送命令
void I2cDisplay::sendCommand(uint8_t cmd) {
    uint8_t data[2] = {0x00, cmd};
    ssize_t bytes = write(i2c_fd, data, 2);
    if (bytes != 2) {
        std::cerr << "发送命令 0x" << std::hex << (int)cmd << " 失败: " << strerror(errno) << std::endl;
    }
    usleep(1000);
}

// 发送缓冲区数据
void I2cDisplay::sendBuffer(const uint8_t* buf, size_t len) {
    uint8_t* tmp = new uint8_t[len + 1];
    tmp[0] = 0x40; // 控制字节，表示数据
    memcpy(tmp + 1, buf, len);
    ssize_t bytes = write(i2c_fd, tmp, len + 1);
    if (bytes != static_cast<ssize_t>(len + 1)) {
        std::cerr << "发送显示数据失败: " << strerror(errno) << std::endl;
    }
    delete[] tmp;
}

// 清空缓冲区
void I2cDisplay::clearBuffer() {
    memset(buffer, 0, sizeof(buffer));
}

// 计算文本宽度
int I2cDisplay::textWidth(const std::string &text) {
    return text.size() * 6;
}

// 在页0绘制单个字符
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

// 单行显示文本（默认在页0）
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

// 在指定页绘制单个字符
void I2cDisplay::drawCharAt(int x, int page, char c) {
    if (c < 32 || c > 126) c = 32;
    int index = c - 32;
    int offset = page * 128; // 每页128字节
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

// 在指定页绘制文本，自动居中
void I2cDisplay::displayTextAt(int page, const std::string &text) {
    int w = text.size() * 6;
    int start_x = (128 - w) / 2;
    int x = start_x;
    for (char c : text) {
        drawCharAt(x, page, c);
        x += 6;
    }
}

// 显示两行文本（例如温湿度数据），分别绘制在页2和页4
void I2cDisplay::displayMultiLine(const std::string &line1, const std::string &line2) {
    clearBuffer();
    displayTextAt(2, line1);
    displayTextAt(4, line2);
    sendBuffer(buffer, sizeof(buffer));
}

// 显示 "INVASION"
void I2cDisplay::displayIntrusion() {
    displayText("INVASION");
    std::cout << "[DISPLAY] INVASION" << std::endl;
    if (eventCallback) eventCallback("INVASION");
}

// 显示 "INVASION" 改为显示两行文字：
// 页0：显示 "INVASION"
// 页2：显示 "Enter password"
// 页4：显示 "to unlock"
void I2cDisplay::displayIntrusion() {
    clearBuffer();
    displayTextAt(0, "INVASION");
    displayTextAt(2, "Enter password");
    displayTextAt(4, "to unlock");
    sendBuffer(buffer, sizeof(buffer));
    std::cout << "[DISPLAY] INVASION" << std::endl;
    if (eventCallback) eventCallback("INVASION");
}


// 显示 SAFE 与温湿度数据：在页0显示 "SAFE"，页2显示温度，页4显示湿度
void I2cDisplay::displaySafeAndDHT(const std::string &tempStr, const std::string &humStr) {
    clearBuffer();
    displayTextAt(0, "SAFE");
    displayTextAt(2, tempStr);
    displayTextAt(4, humStr);
    sendBuffer(buffer, sizeof(buffer));
}

void I2cDisplay::displayWrongPassword() {
    clearBuffer();
    // 这里假设在页0显示第一行文字，页2显示第二行文字
    displayTextAt(0, "Wrong password,");
    displayTextAt(2, "please try again");
    sendBuffer(buffer, sizeof(buffer));
    std::cout << "[DISPLAY] Wrong password, please try again." << std::endl;
}

void I2cDisplay::displayPasswordStars(const std::string &stars) {
    clearBuffer();
    // 这里假设我们在页6显示输入的密码反馈
    displayTextAt(6, stars);
    sendBuffer(buffer, sizeof(buffer));
    std::cout << "[DISPLAY] Password input: " << stars << std::endl;
}

void I2cDisplay::registerEventCallback(std::function<void(const std::string&)> callback) {
    eventCallback = callback;
}


