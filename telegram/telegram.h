#ifndef TELEGRAM_H
#define TELEGRAM_H

#include <string>

// 发送 Telegram 消息的函数声明  
// 参数 token：Bot 的 API Token
// 参数 chat_id：目标聊天的 ID
// 参数 message：要发送的消息文本
// 返回 true 表示发送成功，false 表示发送失败
bool sendTelegramMessage(const std::string &token, const std::string &chat_id, const std::string &message);

#endif // TELEGRAM_H
