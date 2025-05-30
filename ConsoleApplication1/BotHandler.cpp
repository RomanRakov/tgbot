#include "BotHandler.h"
#include <iostream>
#include <cpr/cpr.h>
#include <nlohmann/json.hpp>
#include <thread>
#include <chrono>
#include <random>

using json = nlohmann::json;

std::string cleanJson(const std::string& raw) {
    auto startPos = raw.find_first_of("{[");
    if (startPos == std::string::npos) {
        return "";
    }

    int depth = 0;
    bool inString = false;
    for (size_t i = startPos; i < raw.size(); ++i) {
        char c = raw[i];
        if (c == '"' && (i == 0 || raw[i - 1] != '\\')) {
            inString = !inString;
        }
        if (!inString) {
            if (c == '{' || c == '[') {
                ++depth;
            }
            else if (c == '}' || c == ']') {
                --depth;
                if (depth == 0) {
                    return raw.substr(startPos, i - startPos + 1);
                }
            }
        }
    }
    return raw.substr(startPos);
};


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
    users[chatId] = UserState(); 
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

    else if (data == "similar_products") {
        if (users.count(chatId) && !users[chatId].currentProduct.name.empty()) {
            const auto& target = users[chatId].currentProduct;
            const auto& all = users[chatId].allProducts;

            auto matches = ProductService::findCompatibleProductsAdvanced(target, all);
            if (!matches.empty()) {
                bot.getApi().sendMessage(chatId, u8"🧩 Рекомендуемые товары:");
                for (const auto& p : matches) {
                    std::string url = u8"http://localhost:18080/card?id=" + std::to_string(p.id);
                    std::string msg = u8"• " + p.name + "\n" + p.description + "\n" + url;
                    if (!p.image_url.empty())
                        bot.getApi().sendPhoto(chatId, p.image_url, msg);
                    else
                        bot.getApi().sendMessage(chatId, msg);
                }
                bot.getApi().sendMessage(chatId, u8"Выберите следующее действие:", false, 0, KeyboardFactory::getBackAndMenuKeyboard());

            }

            else {
                bot.getApi().sendMessage(chatId, u8"🔎 Похожие товары не найдены.");
            }
        }
        else {
            bot.getApi().sendMessage(chatId, u8"❌ Нет активного товара.");
        }
    }
    else if (data == "match_products") {
        const auto& state = users[chatId];
        if (!state.currentProduct.name.empty()) {
            auto matches = ProductService::findCompatibleByCategoryFlexible(state.currentProduct, state.allProducts);
            if (!matches.empty()) {
                bot.getApi().sendMessage(chatId, u8"🧩 Совместимые товары по категории:");
                for (const auto& p : matches) {
                    std::string url = u8"http://localhost:18080/card?id=" + std::to_string(p.id);
                    std::string msg = u8"• " + p.name + "\n" + p.description + "\n" + url;
                    if (!p.image_url.empty())
                        bot.getApi().sendPhoto(chatId, p.image_url, msg);
                    else
                        bot.getApi().sendMessage(chatId, msg);
                }
                bot.getApi().sendMessage(chatId, u8"Выберите следующее действие:", false, 0, KeyboardFactory::getBackAndMenuKeyboard());

            }
            else {
                bot.getApi().sendMessage(chatId, u8"❌ Ничего не найдено.");
            }
        }
        else {
            bot.getApi().sendMessage(chatId, u8"❌ Нет активного товара.");
        }
    }
    else if (data == "care_tips") {
        const auto& product = users[chatId].currentProduct;

        if (!product.name.empty()) {
            std::string category = product.category_name;
            std::string tip = u8"ℹ️ Совет по уходу:\n";

            static const std::map<std::string, std::string> careTipsMap = {
                {"Футболки", u8"👕 Стирать при 30°C, вывернув наизнанку. Избегать сушки в машине."},
                {"Рубашки", u8"🧺 Гладить при средней температуре. Хранить на плечиках."},
                {"Платья", u8"🌸 Стирать вручную или в режиме деликатной стирки."},
                {"Кроссовки", u8"👟 Не стирать в машине. Протирать влажной тряпкой."},
                {"Брюки", u8"🧼 Стирка при 30°C. Гладить через ткань."},
                {"default", u8"🔧 Общий совет: читайте ярлык на изделии и избегайте высоких температур."}
            };

            if (careTipsMap.count(category)) {
                tip += careTipsMap.at(category);
            }
            else {
                tip += careTipsMap.at("default");
            }

            bot.getApi().sendMessage(chatId, tip);
            bot.getApi().sendMessage(chatId, u8"Выберите следующее действие:", false, 0, KeyboardFactory::getBackAndMenuKeyboard());
        }
        else {
            bot.getApi().sendMessage(chatId, u8"❌ Нет активного товара.");
        }
    }

    else if (data == "back_to_search") {
        users[chatId].awaitingProductId = true;
        bot.getApi().sendMessage(chatId, u8"📦 Введите ID товара для поиска:");
    }

    else if (data == "back_to_product_menu") {
        bot.getApi().sendMessage(chatId, u8"Выберите действие:", false, 0, KeyboardFactory::getProductMenuKeyboard());
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

        try {
            int productId = std::stoi(input);
            std::string apiUrl = "http://localhost:18080/api/product/" + std::to_string(productId);
            cpr::Response r = cpr::Get(cpr::Url{ apiUrl });

            std::string rawResponse = r.text;
            std::string jsonString = cleanJson(rawResponse);


            if (r.status_code == 200) {
                try {
                    auto j = json::parse(jsonString);
                    Product target;
                    target.id = j.value("id", -1);
                    target.name = j.value("name", "");
                    target.description = j.value("description", "");
                    target.brand = j.value("brand", "");
                    target.image_url = j.value("image_url", "");
                    target.category_name = j.value("category", "");

                    if (target.id > 0 && !target.name.empty()) {
                        std::string productUrl = "http://localhost:18080/card?id=" + std::to_string(productId);
                        std::string messageText = u8"Название: " + target.name + "\n" +
                            u8"Описание: " + target.description + "\n\n" +
                            u8"Ссылка на товар: " + productUrl;

                        if (!target.image_url.empty()) {
                            bot.getApi().sendPhoto(chatId, target.image_url, messageText);
                        }
                        else {
                            bot.getApi().sendMessage(chatId, messageText);
                        }

                        cpr::Response allResp = cpr::Get(cpr::Url{ "http://localhost:18080/api/products" });
                        std::vector<Product> allProducts;

                        if (allResp.status_code == 200) {
                            std::string cleanedAll = cleanJson(allResp.text);
                            auto allJson = json::parse(cleanedAll);
                            for (const auto& item : allJson) {
                                Product p;
                                p.id = item.value("id", -1);
                                p.name = item.value("name", "");
                                p.description = item.value("description", "");
                                p.brand = item.value("brand", "");
                                p.image_url = item.value("image_url", "");
                                p.category_name = item.value("category", "");
                                if (p.id > 0) {
                                    allProducts.push_back(p);
                                }
                            }
                        }
                        else {
                            bot.getApi().sendMessage(chatId, u8"⚠️ Не удалось загрузить список всех товаров.");
                        }

                        users[chatId].currentProduct = target;
                        users[chatId].allProducts = allProducts;

                        auto getProductMenuKeyboard = []() -> TgBot::InlineKeyboardMarkup::Ptr {
                            TgBot::InlineKeyboardMarkup::Ptr keyboard(new TgBot::InlineKeyboardMarkup);

                            std::vector<TgBot::InlineKeyboardButton::Ptr> row1;
                            TgBot::InlineKeyboardButton::Ptr btnSimilar(new TgBot::InlineKeyboardButton);
                            btnSimilar->text = u8"🔄 Похожие товары";
                            btnSimilar->callbackData = "similar_products";
                            row1.push_back(btnSimilar);

                            TgBot::InlineKeyboardButton::Ptr btnMatch(new TgBot::InlineKeyboardButton);
                            btnMatch->text = u8"👕 С чем сочетать";
                            btnMatch->callbackData = "match_products";
                            row1.push_back(btnMatch);
                            keyboard->inlineKeyboard.push_back(row1);


                            std::vector<TgBot::InlineKeyboardButton::Ptr> row2;
                            TgBot::InlineKeyboardButton::Ptr btnCare(new TgBot::InlineKeyboardButton);
                            btnCare->text = u8"🧼 Как ухаживать";
                            btnCare->callbackData = "care_tips";
                            row2.push_back(btnCare);
                            keyboard->inlineKeyboard.push_back(row2);

                            std::vector<TgBot::InlineKeyboardButton::Ptr> rowNav;
                            TgBot::InlineKeyboardButton::Ptr btnBack(new TgBot::InlineKeyboardButton);
                            btnBack->text = u8"⬅️ Назад к поиску";
                            btnBack->callbackData = "back_to_search";
                            rowNav.push_back(btnBack);

                            TgBot::InlineKeyboardButton::Ptr btnMenu(new TgBot::InlineKeyboardButton);
                            btnMenu->text = u8"🏠 Главное меню";
                            btnMenu->callbackData = "main_menu";
                            rowNav.push_back(btnMenu);
                            keyboard->inlineKeyboard.push_back(rowNav);

                            return keyboard;
                            };

                        bot.getApi().sendMessage(chatId, u8"Выберите действие:", false, 0, getProductMenuKeyboard());
                    }
                    else {
                        bot.getApi().sendMessage(chatId, u8"❌ Товар с таким ID не найден.");
                    }

                }
                catch (const std::exception& e) {
                    std::cout << u8"[ERROR] JSON parse error: " << e.what() << std::endl;
                    bot.getApi().sendMessage(chatId, u8"⚠️ Ошибка при обработке данных товара.");
                }
            }
            else {
                bot.getApi().sendMessage(chatId, u8"⚠️ Ошибка при запросе товара с сервера.");
            }

            users[chatId].awaitingProductId = false;
        }
        catch (...) {
            bot.getApi().sendMessage(chatId, u8"❌ Введите корректный номер товара.");
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
