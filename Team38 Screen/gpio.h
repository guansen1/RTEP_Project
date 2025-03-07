#ifdef __linux__
#include <gpiod.h>
#endif

#ifndef GPIO_H
#define GPIO_H

// 如果在 Linux 系统下，则包含 libgpiod 头文件
#ifdef __linux__
#include <gpiod.h>
#endif

#include <iostream>
#include <unordered_map>
#include <vector>
#include <thread>
#include <atomic>
#include <functional>

// 定义 GPIO 模式（可根据需要扩展）
enum GPIOConfig {
    INPUT = 0,
    OUTPUT = 1
    // 如需上拉、下拉或事件模式，可以继续定义
};

class GPIO {
public:
    // 定义事件回调接口
    struct EventCallback {
        virtual void handleEvent(const struct gpiod_line_event& event) = 0;
        virtual ~EventCallback() = default;
    };

    GPIO();
    ~GPIO();

    // 初始化 GPIO：打开芯片和请求必要的引脚
    bool init();

    // 请求指定引脚为某种模式
    bool requestLine(int pin, GPIOConfig config);

    // 读写操作
    int readLine(int pin);
    bool writeLine(int pin, int value);

    // 注册事件回调（例如用于检测边沿事件）
    void registerCallback(EventCallback* cb);

    // 启动与停止事件监听线程
    void start();
    void stop();

private:
    void worker(); // 事件监听线程函数

#ifdef __linux__
    struct gpiod_chip* chip;                          // GPIO 芯片对象
    std::unordered_map<int, struct gpiod_line*> lines; // 存储请求的引脚
#endif

    std::vector<EventCallback*> callbacks; // 注册的事件回调
    std::thread workerThread;              // 事件监听线程
    std::atomic<bool> running;             // 线程运行标志
};

#endif // GPIO_H
