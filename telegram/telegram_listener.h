#ifndef TELEGRAM_LISTENER_H
#define TELEGRAM_LISTENER_H

#include <atomic>
#include <string>
#include <thread>

// **前向声明 Buzzer 类**
class Buzzer;

class TelegramListener {
public:
    // 构造函数
    TelegramListener(const std::string& token, const std::string& chatId, Buzzer& buzzer);
    ~TelegramListener();

    // 启动监听线程
    void start();
    // 停止监听线程
    void stop();

    // 检查是否处于检测模式
    bool isDetectionMode() const;
    // 检查是否允许报警
    bool isAlarmEnabled() const;
    
    void sendTemperatureData(float temperature, float humidity);
    

private:
    // 监听循环
    void run();

    std::string token;
    std::string chatId;
    std::atomic_bool detectionMode; // 控制数据发送
    std::atomic_bool alarmEnabled;  // 控制 PIR 是否触发蜂鸣器
    std::atomic_bool running;
    std::thread listenerThread;
    Buzzer& buzzer;  // **引用 Buzzer 实例**
};

#endif // TELEGRAM_LISTENER_H
