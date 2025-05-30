#ifndef BOT_HANDLER_H
#define BOT_HANDLER_H

#include <tgbot/tgbot.h>
#include "UserService.h"
#include "StyleService.h"
#include "KeyboardFactory.h"
#include "ProductService.h"
#include "QuestionnaireService.h"

#include <map>

struct UserState {
    int step = 0;
    int lastQuestionMessageId = 0;
    bool awaitingProductId = false;
    Product currentProduct;
    std::vector<Product> allProducts;
};

class BotHandler {
public:
    BotHandler(const std::string& token, sqlite3* db);

    void start();

private:
    TgBot::Bot bot;
    sqlite3* db;

    UserService userService;
    StyleService styleService;
    QuestionnaireService questionnaire;

    std::map<int64_t, UserState> users;
    std::map<int64_t, int> previousQuestionMessageIds;

    void setupHandlers();
    void handleStart(TgBot::Message::Ptr message);
    void handleCallbackQuery(TgBot::CallbackQuery::Ptr query);
    void handleMessage(TgBot::Message::Ptr message);
    void handleDailyTips(); 
};

#endif 
#pragma once
