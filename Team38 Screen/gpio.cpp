#include "gpio.h"

#ifdef __linux__
#include <gpiod.h>
#include <cstring>
#endif

GPIO::GPIO() : running(false)
{
#ifdef __linux__
    // 尝试打开默认的 gpiochip0（Linux 下有效）
    chip = gpiod_chip_open_by_name("gpiochip0");
    if (!chip) {
        std::cerr << "无法打开 GPIO 芯片 gpiochip0" << std::endl;
    }
#endif
}

GPIO::~GPIO()
{
    stop();
#ifdef __linux__
    for (auto& pair : lines) {
        if(pair.second) {
            gpiod_line_release(pair.second);
        }
    }
    if (chip) {
        gpiod_chip_close(chip);
    }
#endif
}

bool GPIO::init()
{
#ifdef __linux__
    return (chip != nullptr);
#else
    // 非 Linux 平台返回 false（仅用于语法检查）
    return false;
#endif
}

bool GPIO::requestLine(int pin, GPIOConfig config)
{
#ifdef __linux__
    if (!chip) return false;
    struct gpiod_line* line = gpiod_chip_get_line(chip, pin);
    if (!line) {
        std::cerr << "无法获取 GPIO 引脚 " << pin << std::endl;
        return false;
    }
    int ret = -1;
    if (config == OUTPUT) {
        ret = gpiod_line_request_output(line, "gpio", 0);
    } else if (config == INPUT) {
        ret = gpiod_line_request_input(line, "gpio");
    }
    if (ret < 0) {
        std::cerr << "请求 GPIO 引脚 " << pin << " 模式失败" << std::endl;
        return false;
    }
    lines[pin] = line;
    return true;
#else
    return false;
#endif
}

int GPIO::readLine(int pin)
{
#ifdef __linux__
    if (lines.find(pin) == lines.end()) {
        std::cerr << "GPIO 引脚 " << pin << " 未请求" << std::endl;
        return -1;
    }
    return gpiod_line_get_value(lines[pin]);
#else
    return -1;
#endif
}

bool GPIO::writeLine(int pin, int value)
{
#ifdef __linux__
    if (lines.find(pin) == lines.end()) {
        std::cerr << "GPIO 引脚 " << pin << " 未请求" << std::endl;
        return false;
    }
    if (gpiod_line_set_value(lines[pin], value) < 0) {
        std::cerr << "写入 GPIO 引脚 " << pin << " 失败" << std::endl;
        return false;
    }
    return true;
#else
    return false;
#endif
}

void GPIO::registerCallback(EventCallback* cb)
{
    callbacks.push_back(cb);
}

void GPIO::start()
{
    running = true;
    workerThread = std::thread(&GPIO::worker, this);
}

void GPIO::stop()
{
    running = false;
    if (workerThread.joinable()) {
        workerThread.join();
    }
}

void GPIO::worker()
{
#ifdef __linux__
    // 简单示例：遍历所有已请求的引脚，等待上升沿事件（超时100ms）
    while (running) {
        for (auto& pair : lines) {
            struct gpiod_line_event event;
            struct timespec timeout = {0, 100 * 1000000}; // 100ms
            int ret = gpiod_line_event_wait(pair.second, &timeout);
            if (ret > 0) {
                if (gpiod_line_event_read(pair.second, &event) == 0) {
                    // 调用所有注册的回调处理事件
                    for (auto cb : callbacks) {
                        cb->handleEvent(event);
                    }
                }
            }
        }
        std::this_thread::sleep_for(milliseconds(10));
    }
#endif
}
