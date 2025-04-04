#ifndef TELEGRAM_H
#define TELEGRAM_H

#include <string>

// Function declaration for sending Telegram messages  
// Parameter token: the Bot's API Token
// Parameter chat_id: the ID of the target chat.
// Parameter message: the text of the message to be sent
// Returns true if the message was sent successfully, false if it was not.
bool sendTelegramMessage(const std::string &token, const std::string &chat_id, const std::string &message);

#endif // TELEGRAM_H
