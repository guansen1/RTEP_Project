#include <wiringPi.h>
#include <wiringPiI2C.h>
#include <iostream>
#include "ADS1115.h"
#include "MQ2Sensor.h"

// 定义全局指针，方便在中断回调中访问传感器对象
MQ2Sensor* g_mq2 = nullptr;

// 中断服务函数，当 ADS1115 的 ALERT/RDY 引脚触发中断时调用
void sensorDataReadyInterrupt() {
    if (g_mq2) {
        int16_t sensorValue = g_mq2->getSensorReading();
        std::cout << "MQ-2 Sensor Reading: " << sensorValue << std::endl;
        // 根据读取的数据进行判断，例如超过某个阈值则触发报警
    }
}

int main() {
    // 初始化 wiringPi，使用 Broadcom GPIO 编号
    if (wiringPiSetupGpio() == -1) {
        std::cerr << "Failed to initialize wiringPi." << std::endl;
        return -1;
    }

    // 使用 wiringPiI2CSetup 打开 I2C 设备，ADS1115 默认地址 0x48
    int i2c_fd = wiringPiI2CSetup(0x48);
    if (i2c_fd == -1) {
        std::cerr << "Failed to open I2C device." << std::endl;
        return -1;
    }

    // 创建 ADS1115 对象并初始化
    ADS1115 ads(i2c_fd);
    if (!ads.init()) {
        std::cerr << "Failed to initialize ADS1115." << std::endl;
        return -1;
    }

    // 如有需要，可配置 ADS1115 的比较器以产生中断（此处省略具体配置参数）
    // ads.configureComparator(threshold, config);

    // 创建 MQ-2 传感器对象
    MQ2Sensor mq2(&ads);
    g_mq2 = &mq2; // 为全局指针赋值，便于中断回调访问

    // 配置与 ADS1115 ALERT/RDY 引脚相连的 GPIO 口
    // 假设 ALERT 引脚连接在 GPIO 17（BCM 编号），wiringPi 使用 BCM 编号时直接使用该编号
    int alertPin = 17;
    pinMode(alertPin, INPUT);
    pullUpDnControl(alertPin, PUD_UP);  // 根据实际需要选择上拉或下拉

    // 设置 GPIO 中断，触发方式根据 ADS1115 输出信号的电平变化而定（这里示例使用下降沿触发）
    if (wiringPiISR(alertPin, INT_EDGE_FALLING, &sensorDataReadyInterrupt) < 0) {
        std::cerr << "Unable to setup ISR." << std::endl;
        return -1;
    }

    std::cout << "MQ-2 sensor monitoring started. Waiting for data ready interrupts..." << std::endl;

    // 主循环中无需轮询，中断处理会自动调用回调函数
    while (true) {
        delay(1000); // 主循环中仅作简单延时
    }

    return 0;
}
