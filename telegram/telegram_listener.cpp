#include "telegram_listener.h"
#include "telegram.h"
#include <curl/curl.h>
#include <sstream>
#include <iostream>
#include <chrono>
#include "buzzer/buzzer.h"

// 用于收集 libcurl 返回的数据
static size_t WriteCallback(void *contents, size_t size, size_t nmemb, void *userp) {
    size_t totalSize = size * nmemb;
    std::string* str = static_cast<std::string*>(userp);
    str->append(static_cast<char*>(contents), totalSize);
    return totalSize;
}

TelegramListener::TelegramListener(const std::string& token, const std::string& chatId, Buzzer& buzzer)
    : token(token), chatId(chatId), detectionMode(false), alarmEnabled(true), running(false), buzzer(buzzer) {}

TelegramListener::~TelegramListener() {
    stop();
}

void TelegramListener::sendTemperatureData(float temperature, float humidity) {
    if (detectionMode) {
        std::ostringstream oss;
        oss << "当前温度: " << temperature << "°C, 湿度: " << humidity << "%";
        std::string message = oss.str();
        bool ret = sendTelegramMessage(token, chatId, message);
        if (!ret) {
            std::cerr << "发送 Telegram 消息失败！" << std::endl;
        } else {
            std::cout << "发送 Telegram 消息成功: " << message << std::endl;
        }
    }
}


void TelegramListener::start() {
    running = true;
    listenerThread = std::thread(&TelegramListener::run, this);
}

void TelegramListener::stop() {
    running = false;
    if (listenerThread.joinable())
        listenerThread.join();
}

bool TelegramListener::isDetectionMode() const {
    return detectionMode.load();
}

bool TelegramListener::isAlarmEnabled() const {
    return alarmEnabled.load();
}

void TelegramListener::run() {
    long last_update_id = 0;
    while (running) {
        CURL *curl = curl_easy_init();
        if (!curl) {
            std::cerr << "无法初始化 curl" << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(2));
            continue;
        }
        std::stringstream urlStream;
        urlStream << "https://api.telegram.org/bot" << token 
                  << "/getUpdates?timeout=10";
        if (last_update_id > 0) {
            urlStream << "&offset=" << (last_update_id + 1);
        }
        std::string url = urlStream.str();

        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

        std::string response;
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

        CURLcode res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            std::cerr << "getUpdates 失败: " << curl_easy_strerror(res) << std::endl;
            curl_easy_cleanup(curl);
            std::this_thread::sleep_for(std::chrono::seconds(2));
            continue;
        }
        curl_easy_cleanup(curl);

        // 处理 "start detection" 命令
        if (response.find("start detection") != std::string::npos) {
            std::cout << "接收到 'start detection' 命令，开始发送数据" << std::endl;
            detectionMode = true;
        } 
        // 处理 "stop detection" 命令
        else if (response.find("stop detection") != std::string::npos) {
            std::cout << "接收到 'stop detection' 命令，停止发送数据" << std::endl;
            detectionMode = false;
        } 
        // 处理 "deactivate the alarm" 命令
        else if (response.find("deactivate the alarm") != std::string::npos) {
            std::cout << "接收到 'deactivate the alarm' 命令，停止蜂鸣器报警" << std::endl;
            alarmEnabled = false;
            buzzer.disable();  // 停止蜂鸣器
        }

        // 更新 last_update_id，防止重复处理消息
        size_t pos = response.find("\"update_id\":");
        while (pos != std::string::npos) {
            size_t start = pos + std::string("\"update_id\":").length();
            size_t end = response.find(",", start);
            if (end == std::string::npos)
                break;
            long update_id = std::stol(response.substr(start, end - start));
            if (update_id > last_update_id) {
                last_update_id = update_id;
            }
            pos = response.find("\"update_id\":", end);
        }
        std::this_thread::sleep_for(std::chrono::seconds(2));
    }
}
