#ifndef DISPLAY_HPP
#define DISPLAY_HPP

#include <string>
#include <mutex>
#include <cstdint>

class SSD1306Display {
public:
    SSD1306Display();
    ~SSD1306Display();

    // 初始化 OLED 显示器（打开 I2C、发送初始化序列）
    bool initialize();

    // 清空显示内容（清空缓冲区并刷新屏幕）
    bool clear();

    // 在指定页（0~7）和起始列（0~127）处显示文本（每个字符宽度为6像素：5+1空白）
    bool updateText(const std::string &text, int page, int col);

private:
    // 发送单个命令字节
    bool writeCommand(uint8_t cmd);
    // 发送数据块
    bool writeData(const uint8_t *data, size_t length);
    // 将内部缓冲区数据写入 OLED 显示器
    bool updateBuffer();

    int i2c_fd;              // I2C 文件描述符
    std::mutex displayMutex; // 保护显示操作的互斥锁

    static const int WIDTH = 128;
    static const int HEIGHT = 64;
    static const int PAGES = HEIGHT / 8; // 每页8行，总共8页

    uint8_t buffer[WIDTH * (HEIGHT / 8)]; // 显示缓冲区

    // 内部使用的 5x7 字体（支持 ASCII 32~127）
    static const uint8_t font5x7[96][5];
};

#endif // DISPLAY_HPP
