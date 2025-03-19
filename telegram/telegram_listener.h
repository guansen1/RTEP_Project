#ifndef TELEGRAM_LISTENER_H
#define TELEGRAM_LISTENER_H

#include <atomic>
#include <string>
#include <thread>

class TelegramListener {
public:
    // 构造函数
    TelegramListener(const std::string& token, const std::string& chatId);
    ~TelegramListener();

    // 启动监听线程
    void start();
    // 停止监听线程
    void stop();

    // 检查是否处于检测模式
    bool isDetectionMode() const;

private:
    // 监听循环
    void run();

    std::string token;
    std::string chatId;
    std::atomic_bool detectionMode;
    std::atomic_bool running;
    std::thread listenerThread;
};

#endif // TELEGRAM_LISTENER_H
