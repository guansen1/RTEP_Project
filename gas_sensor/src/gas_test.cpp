#include <iostream>
#include <fstream>
#include <ctime>
#include <wiringPi.h>

int main()
{
    // 使用 wiringPi 编号方式初始化
    if (wiringPiSetup() == -1) {
        std::cerr << "Failed to initialize wiringPi!" << std::endl;
        return 1;
    }

    // 假设树莓派 GPIO17 (BCM 17) 对应 wiringPi 的 pin 0
    // 请根据实际情况调整引脚映射
    int smokePin = 0;
    pinMode(smokePin, INPUT);

    // 以追加方式（ios::app）打开日志文件 gas_output
    // 如果需要每次运行程序都从头覆盖，可去掉 ios::app
    std::ofstream logFile("gas_output", std::ios::app);
    if (!logFile.is_open()) {
        std::cerr << "Failed to open 'gas_output' file for logging!" << std::endl;
        return 1;
    }

    std::cout << "Starting to monitor gas sensor, writing logs to 'gas_output'..." << std::endl;

    while (true)
    {
        // 读取数字引脚状态，0 或 1
        int sensorVal = digitalRead(smokePin);

        // 获取当前系统时间
        std::time_t now = std::time(nullptr);
        char timeBuf[64];
        std::strftime(timeBuf, sizeof(timeBuf), "%Y-%m-%d %H:%M:%S", std::localtime(&now));

        // 在终端上输出
        if (sensorVal == HIGH) {
            std::cout << "[TERMINAL] " << timeBuf << " : Smoke detected!" << std::endl;
        } else {
            std::cout << "[TERMINAL] " << timeBuf << " : No smoke." << std::endl;
        }

        // 同时写入文件
        if (sensorVal == HIGH) {
            logFile << "[LOG] " << timeBuf << " : Smoke detected!" << std::endl;
        } else {
            logFile << "[LOG] " << timeBuf << " : No smoke." << std::endl;
        }

        // 为了安全（在异常退出时也能保存），可以及时刷新文件缓冲
        logFile.flush();

        // 每隔一秒读取一次
        delay(1000);
    }

    // 如果程序可以正常结束，则关闭文件
    logFile.close();

    return 0;
}
