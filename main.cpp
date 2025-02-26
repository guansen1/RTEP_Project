#include <iostream>
#include <thread>
#include <atomic>
#include <condition_variable>
#include <chrono>
#include <mutex>
#include <fstream>

// 共享变量
std::atomic<bool> alarm_triggered(false); // 是否触发报警
std::atomic<bool> running(true);          // 程序是否运行
std::mutex mtx;
std::condition_variable cv; 

// 🔹 传感器线程（模拟 PIR 传感器 & 温湿度传感器）
void sensorThread() {
    while (running) {
        std::this_thread::sleep_for(std::chrono::seconds(5)); // 模拟传感器采样间隔
        int motion = rand() % 2;  // 0: 无入侵, 1: 触发入侵
        float temperature = 20 + rand() % 5; // 模拟温度 (20~24°C)
        float humidity = 50 + rand() % 10;   // 模拟湿度 (50~59%)

        std::lock_guard<std::mutex> lock(mtx);
        std::cout << "[传感器] 温度: " << temperature << "°C, 湿度: " << humidity << "%\n";
        if (motion) {
            std::cout << "[传感器] 入侵检测: 触发报警！\n";
            alarm_triggered = true;
            cv.notify_all(); // 唤醒报警线程
        }
    }
}

// 🔹 报警线程（当检测到入侵时触发警报）
void alarmThread() {
    while (running) {
        std::unique_lock<std::mutex> lock(mtx);
        cv.wait(lock, [] { return alarm_triggered.load() || !running; }); // 休眠等待触发
        if (!running) break;

        std::cout << "[报警] 警报启动！\n";
        std::this_thread::sleep_for(std::chrono::seconds(3)); // 模拟警报时间
        std::cout << "[报警] 警报结束\n";
    }
}

// 🔹 用户输入线程（模拟键盘输入，解除/触发警报）
void userInputThread() {
    while (running) {
        char cmd;
        std::cout << "\n[控制台] 输入 'a' 触发警报, 'd' 解除警报, 'q' 退出: ";
        std::cin >> cmd;

        std::lock_guard<std::mutex> lock(mtx);
        if (cmd == 'a') {
            std::cout << "[用户] 手动触发警报！\n";
            alarm_triggered = true;
            cv.notify_all();
        } else if (cmd == 'd') {
            std::cout << "[用户] 解除警报。\n";
            alarm_triggered = false;
        } else if (cmd == 'q') {
            std::cout << "[退出] 终止程序。\n";
            running = false;
            cv.notify_all(); // 唤醒所有线程
            break;
        }
    }
}

// 🔹 日志记录线程（写入警报历史）
void logThread() {
    std::ofstream logFile("alarm_log.txt", std::ios::app);
    while (running) {
        std::this_thread::sleep_for(std::chrono::seconds(5)); // 模拟日志间隔
        if (alarm_triggered) {
            logFile << "[日志] 警报触发，时间戳: " << time(nullptr) << std::endl;
            std::cout << "[日志] 记录警报触发。\n";
        }
    }
    logFile.close();
}

// 🔹 主程序（管理线程）
int main() {
    std::cout << "家用迷你报警系统启动！\n";

    std::thread t1(sensorThread);
    std::thread t2(alarmThread);
    std::thread t3(userInputThread);
    std::thread t4(logThread);

    // 等待线程结束
    t1.join();
    t2.join();
    t3.join();
    t4.join();

    std::cout << "退出程序。\n";
    return 0;
}
