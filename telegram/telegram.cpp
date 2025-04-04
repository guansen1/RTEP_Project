#include "telegram.h"
#include <curl/curl.h>
#include <iostream>
#include <sstream>

bool sendTelegramMessage(const std::string &token, const std::string &chat_id, const std::string &message) {
    CURL *curl = curl_easy_init();
    if (!curl) {
        std::cerr << "cannot initialize curl" << std::endl;
        return false;
    }

    // URL encoding of messages
    char *escapedMessage = curl_easy_escape(curl, message.c_str(), message.length());
    if (!escapedMessage) {
        std::cerr << "Message URL encoding failure" << std::endl;
        curl_easy_cleanup(curl);
        return false;
    }

    // Constructing the request URL
    std::stringstream url;
    url << "https://api.telegram.org/bot" << token << "/sendMessage?chat_id=" << chat_id
        << "&text=" << escapedMessage;

    //  Releasing the encoded message
    curl_free(escapedMessage);

    // Setting the curl parameter  
    curl_easy_setopt(curl, CURLOPT_URL, url.str().c_str());
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

    // Send HTTP request
    CURLcode res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
        std::cerr << "curl_easy_perform() fail: " << curl_easy_strerror(res) << std::endl;
        curl_easy_cleanup(curl);
        return false;
    }

    curl_easy_cleanup(curl);
    return true;
}
