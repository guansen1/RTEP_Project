#include "display.hpp"
#include <chrono>
#include <thread>
#include <iostream>

int main() {
    // 创建 SSD1306 显示对象
    SSD1306Display display;
    
    // 清空显示内容
    if (!display.clear()) {
        std::cerr << "清屏失败！" << std::endl;
        return -1;
    }
    
    // 在第0页、起始列0处显示文本
    if (!display.updateText("Hello, SSD1306!", 0, 0)) {
        std::cerr << "显示文本失败！" << std::endl;
        return -1;
    }
    
    // 保持运行10秒钟以便观察显示效果
    std::this_thread::sleep_for(std::chrono::seconds(10));
    return 0;
}
