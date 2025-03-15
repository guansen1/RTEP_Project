#include "telegram.h"
#include <curl/curl.h>
#include <iostream>
#include <sstream>

bool sendTelegramMessage(const std::string &token, const std::string &chat_id, const std::string &message) {
    CURL *curl = curl_easy_init();
    if (!curl) {
        std::cerr << "无法初始化 curl" << std::endl;
        return false;
    }

    // 对消息进行 URL 编码
    char *escapedMessage = curl_easy_escape(curl, message.c_str(), message.length());
    if (!escapedMessage) {
        std::cerr << "消息 URL 编码失败" << std::endl;
        curl_easy_cleanup(curl);
        return false;
    }

    // 构建请求 URL
    std::stringstream url;
    url << "https://api.telegram.org/bot" << token << "/sendMessage?chat_id=" << chat_id
        << "&text=" << escapedMessage;

    // 释放编码后的消息
    curl_free(escapedMessage);

    // 设置 curl 参数  
    curl_easy_setopt(curl, CURLOPT_URL, url.str().c_str());
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

    // 发送 HTTP 请求
    CURLcode res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
        std::cerr << "curl_easy_perform() 失败: " << curl_easy_strerror(res) << std::endl;
        curl_easy_cleanup(curl);
        return false;
    }

    curl_easy_cleanup(curl);
    return true;
}
