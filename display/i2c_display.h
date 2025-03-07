#ifndef I2C_DISPLAY_H
#define I2C_DISPLAY_H

#include <string>
#include <functional>
#include <cstdint>

class I2cDisplay {
public:
    // 获取单例实例
    static I2cDisplay& getInstance();

    // 初始化 SSD1306（打开 I2C 总线并发送初始化命令）
    void init();

    // 检测到人时调用，显示 "INVASION" 居中
    void displayIntrusion();

    // 未检测到人时调用，显示 "SAFE" 居中
    void displaySafe();

    // 注册事件回调（选填，用于调试联动）
    void registerEventCallback(std::function<void(const std::string&)> callback);

private:
    I2cDisplay();
    ~I2cDisplay();
    I2cDisplay(const I2cDisplay&) = delete;
    I2cDisplay& operator=(const I2cDisplay&) = delete;

    // 发送单个命令到 SSD1306（控制字节 0x00）
    void sendCommand(uint8_t cmd);
    // 发送整个显示缓冲区到 SSD1306（控制字节 0x40）
    void sendBuffer(const uint8_t* buf, size_t len);

    // 清空本地显示缓冲区
    void clearBuffer();

    // 将字符串绘制到缓冲区，并居中显示在屏幕最上方（假设单行显示，屏幕宽 128 像素）
    void displayText(const std::string &text);

    // 绘制单个字符到缓冲区的 page=0（顶端8像素），使用 5×7 字体，每字符宽 5 像素+1 列间隔
    void drawChar(int x, char c);

    // 计算字符串总宽度（每字符占6像素）
    int textWidth(const std::string &text);

private:
    int i2c_fd; // I2C 文件描述符
    std::function<void(const std::string&)> eventCallback;
    uint8_t buffer[1024]; // 显示缓冲区：128 x 64 / 8 = 1024字节
};

#endif // I2C_DISPLAY_H
