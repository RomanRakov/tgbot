#include "BotHandler.h"
#include <iostream>
#include <cpr/cpr.h>
#include <nlohmann/json.hpp>
#include <thread>
#include <chrono>
#include <random>

using json = nlohmann::json;

BotHandler::BotHandler(const std::string& token, sqlite3* db)
    : bot(token), db(db),
    userService(db),
    styleService(),
    questionnaire() {
    setupHandlers();
}

void BotHandler::start() {
    std::thread tipThread([this]() {
        handleDailyTips();
        });
    tipThread.detach();

    try {
        bot.getApi().deleteWebhook();
        std::cout << "Webhook deleted." << std::endl;

        std::cout << "Bot username: " << bot.getApi().getMe()->username << std::endl;
        TgBot::TgLongPoll longPoll(bot);
        while (true) {
            std::cout << "Polling..." << std::endl;
            longPoll.start();
        }
    }
    catch (const TgBot::TgException& e) {
        std::cerr << "Bot error: " << e.what() << std::endl;
    }
}

void BotHandler::setupHandlers() {
    bot.getEvents().onCommand("start", [this](TgBot::Message::Ptr message) {
        handleStart(message);
        });

    bot.getEvents().onCallbackQuery([this](TgBot::CallbackQuery::Ptr query) {
        handleCallbackQuery(query);
        });

    bot.getEvents().onAnyMessage([this](TgBot::Message::Ptr message) {
        handleMessage(message);
        });
}

void BotHandler::handleStart(TgBot::Message::Ptr message) {
    int64_t chatId = message->chat->id;
    userService.addUser(chatId);
    users[chatId] = UserState(); // сбросить состояние
    bot.getApi().sendMessage(chatId,
        u8"✨ Привет! Я твой личный помощник по стилю.\n\nЧто ты хочешь сделать?",
        false, 0, KeyboardFactory::getMainMenuKeyboard());
}

void BotHandler::handleCallbackQuery(TgBot::CallbackQuery::Ptr query) {
    int64_t chatId = query->message->chat->id;
    int msgId = query->message->messageId;
    const std::string& data = query->data;

    if (data == "main_menu") {
        users[chatId] = UserState();
        bot.getApi().editMessageText(
            u8"✨ Привет! Я твой личный помощник по стилю.\n\nЧто ты хочешь сделать?",
            chatId, msgId, "", "Markdown", false, KeyboardFactory::getMainMenuKeyboard());
    }
    else if (data == "search_product") {
        users[chatId].awaitingProductId = true;
        bot.getApi().sendMessage(chatId, u8"📦 Введите ID товара для поиска:");
    }
    else if (data == "start_test") {
        users[chatId] = UserState();
        questionnaire.resetUser(chatId);
        const Question& q = questionnaire.getQuestion(0);
        TgBot::Message::Ptr sent = bot.getApi().sendMessage(
            chatId, q.text + u8"\n\n" +
            q.options[0] + "\n" + q.options[1] + "\n" + q.options[2] + "\n" + q.options[3],
            false, 0, KeyboardFactory::getAnswerButtons(false));
        users[chatId].lastQuestionMessageId = sent->messageId;
        previousQuestionMessageIds[chatId] = sent->messageId;
    }
    else if (data.rfind("answer_", 0) == 0) {
        if (users[chatId].lastQuestionMessageId != msgId) {
            bot.getApi().answerCallbackQuery(query->id, u8"Ответьте на последний вопрос.", true);
            return;
        }
        char answer = data[7];
        questionnaire.recordAnswer(chatId, answer);
        int step = ++users[chatId].step;

        try {
            bot.getApi().deleteMessage(chatId, previousQuestionMessageIds[chatId]);
        }
        catch (...) {}

        if (step >= questionnaire.getTotalQuestions()) {
            char style = questionnaire.calculateResult(chatId);
            std::string styleName;
            std::string styleDescription;
            switch (style) {
            case 'A': styleName = u8"Классический стиль";
                styleDescription = u8"Строгие линии, сдержанные цвета..."; break;
            case 'B': styleName = u8"Спортивный стиль";
                styleDescription = u8"Функциональная одежда..."; break;
            case 'C': styleName = u8"Романтический стиль";
                styleDescription = u8"Пастельные тона, лёгкие ткани..."; break;
            case 'D': styleName = u8"Драматический стиль";
                styleDescription = u8"Яркие цвета, контрасты..."; break;
            }

            std::stringstream ss;
            ss << u8"✅ Готово!\n\n" << styleName << "\n\n" << styleDescription << "\n\n";
            auto percentages = questionnaire.getAnswerPercentages(chatId);
            ss << u8"📊 Распределение:\n";
            ss << "A: " << std::fixed << std::setprecision(1) << percentages['A'] << "%\n";
            ss << "B: " << percentages['B'] << "%\n";
            ss << "C: " << percentages['C'] << "%\n";
            ss << "D: " << percentages['D'] << "%\n";

            userService.setUserStyle(chatId, style);
            bot.getApi().sendMessage(chatId, ss.str(), false, 0, KeyboardFactory::getMainMenuKeyboard());

            questionnaire.resetUser(chatId);
            users.erase(chatId);
            previousQuestionMessageIds.erase(chatId);
        }
        else {
            const auto& q = questionnaire.getQuestion(step);
            TgBot::Message::Ptr sent = bot.getApi().sendMessage(
                chatId, q.text + u8"\n\n" +
                q.options[0] + "\n" + q.options[1] + "\n" + q.options[2] + "\n" + q.options[3],
                false, 0, KeyboardFactory::getAnswerButtons(step > 0));
            users[chatId].lastQuestionMessageId = sent->messageId;
            previousQuestionMessageIds[chatId] = sent->messageId;
        }
    }
    else if (data == "settings") {
        bool tipsEnabled = userService.isDailyTipsEnabled(chatId);
        bot.getApi().editMessageText(
            u8"⚙️ Настройки:\n\nЗдесь ты можешь настроить параметры бота.",
            chatId, msgId, "", "Markdown", false,
            KeyboardFactory::getSettingsKeyboard(tipsEnabled));
    }
    else if (data == "toggle_daily_tips") {
        bool current = userService.isDailyTipsEnabled(chatId);
        userService.setDailyTipsEnabled(chatId, !current);
        bot.getApi().editMessageText(
            u8"⚙️ Настройки обновлены.",
            chatId, msgId, "", "Markdown", false,
            KeyboardFactory::getSettingsKeyboard(!current));
    }

    bot.getApi().answerCallbackQuery(query->id);
}

void BotHandler::handleMessage(TgBot::Message::Ptr message) {
    int64_t chatId = message->chat->id;
    if (message->text.empty()) return;

    if (users[chatId].awaitingProductId) {
        std::string input = message->text;
        input.erase(remove_if(input.begin(), input.end(), ::isspace), input.end());

        if (!std::all_of(input.begin(), input.end(), ::isdigit)) {
            bot.getApi().sendMessage(chatId, u8"❌ Введите корректный ID (цифры).");
            return;
        }

        int productId = std::stoi(input);
        std::string url = "http://localhost:18080/api/product/" + std::to_string(productId);
        cpr::Response r = cpr::Get(cpr::Url{ url });

        if (r.status_code == 200) {
            try {
                std::string jsonStr = ProductService::cleanJson(r.text);
                auto j = json::parse(jsonStr);
                Product p;
                p.id = j.value("id", -1);
                p.name = j.value("name", "");
                p.description = j.value("description", "");
                p.brand = j.value("brand", "");
                p.image_url = j.value("image_url", "");
                p.category_name = j.value("category", "");

                if (p.id <= 0 || p.name.empty()) {
                    bot.getApi().sendMessage(chatId, u8"❌ Товар не найден.");
                    return;
                }

                std::string card = "http://localhost:18080/card?id=" + std::to_string(p.id);
                std::string msg = u8"Название: " + p.name + "\nОписание: " + p.description + "\n\nСсылка: " + card;
                if (!p.image_url.empty())
                    bot.getApi().sendPhoto(chatId, p.image_url, msg);
                else
                    bot.getApi().sendMessage(chatId, msg);

                cpr::Response allResp = cpr::Get(cpr::Url{ "http://localhost:18080/api/products" });
                std::vector<Product> all;

                if (allResp.status_code == 200) {
                    auto arr = json::parse(ProductService::cleanJson(allResp.text));
                    for (const auto& item : arr) {
                        Product q;
                        q.id = item.value("id", -1);
                        q.name = item.value("name", "");
                        q.description = item.value("description", "");
                        q.brand = item.value("brand", "");
                        q.image_url = item.value("image_url", "");
                        q.category_name = item.value("category", "");
                        if (q.id > 0) all.push_back(q);
                    }
                }

                users[chatId].currentProduct = p;
                users[chatId].allProducts = all;
                users[chatId].awaitingProductId = false;

                bot.getApi().sendMessage(chatId, u8"Выберите действие:", false, 0, KeyboardFactory::getProductMenuKeyboard());

            }
            catch (...) {
                bot.getApi().sendMessage(chatId, u8"⚠️ Ошибка при обработке JSON.");
            }
        }
        else {
            bot.getApi().sendMessage(chatId, u8"⚠️ Ошибка получения товара с сервера.");
        }
    }
    else {
        bot.getApi().sendMessage(chatId, u8"Нажми /start, чтобы начать.", false, 0, KeyboardFactory::getMainMenuKeyboard());
    }
}

void BotHandler::handleDailyTips() {
    while (true) {
        std::time_t t = std::time(nullptr) + 3 * 3600;
        std::tm now;
        gmtime_s(&now, &t);
        if (now.tm_hour == 20 && now.tm_min == 0) {
            std::string sql = "SELECT chat_id, style FROM users WHERE daily_tips_enabled = 1 AND style IS NOT NULL;";
            sqlite3_stmt* stmt;
            if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
                while (sqlite3_step(stmt) == SQLITE_ROW) {
                    int64_t chatId = sqlite3_column_int64(stmt, 0);
                    const char* styleChar = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
                    if (styleChar && strlen(styleChar) == 1) {
                        char style = styleChar[0];
                        const auto& tips = styleService.getTips(style);
                        if (!tips.empty()) {
                            std::string tip = tips[rand() % tips.size()];
                            try {
                                bot.getApi().sendMessage(chatId, tip);
                            }
                            catch (...) {}
                        }
                    }
                }
                sqlite3_finalize(stmt);
            }
            std::this_thread::sleep_for(std::chrono::seconds(60));
        }
        std::this_thread::sleep_for(std::chrono::seconds(30));
    }
}
