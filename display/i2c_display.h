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
    virtual void init();

    // 单行显示文本（默认在页0，居中显示）
    virtual void displayText(const std::string &text);

    // 在指定页绘制文本，自动水平居中
    virtual void displayTextAt(int page, const std::string &text);

    // 显示两行文本（例如温湿度数据），分别绘制在指定页（例如页2和页4）
    virtual void displayMultiLine(const std::string &line1, const std::string &line2);

    // 显示 "INVASION"（居中显示，覆盖整个屏幕）
    virtual void displayIntrusion();

    // 显示 "SAFE"（仅显示 SAFE，不更新温湿度数据）
    virtual void displaySafe();

    // 在 SAFE 状态下同时显示 "SAFE" 与温湿度数据，
    // 在页0显示 "SAFE"，在页2显示温度，在页4显示湿度
    virtual void displaySafeAndDHT(const std::string &tempStr, const std::string &humStr);

    // 注册事件回调（选填，用于调试或联动）
    virtual void registerEventCallback(std::function<void(const std::string&)> callback);
    virtual ~I2cDisplay();
protected:
// 发送单个命令到 SSD1306（控制字节 0x00）
    void sendCommand(uint8_t cmd);
    // 发送整个显示缓冲区到 SSD1306（控制字节 0x40）
    void sendBuffer(const uint8_t* buf, size_t len);

    // 清空本地显示缓冲区
    void clearBuffer();

    // 辅助函数：计算字符串宽度（每字符 5 像素 + 1 像素间隔）
    int textWidth(const std::string &text);
    
    // 辅助函数：在页0绘制单个字符（5×7 字体）
    void drawChar(int x, char c);
    
    // 新增：在指定页绘制单个字符（page 取值 0～7，每页8像素）
    void drawCharAt(int x, int page, char c);

private:
    I2cDisplay();
    I2cDisplay(const I2cDisplay&) = delete;
    I2cDisplay& operator=(const I2cDisplay&) = delete;

    int i2c_fd; // I2C 文件描述符
    std::function<void(const std::string&)> eventCallback;
    uint8_t buffer[1024]; // 显示缓冲区：128 x 64 / 8 = 1024 字节
};

#endif // I2C_DISPLAY_H
