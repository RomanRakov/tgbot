﻿#include <stdio.h>
#include <tgbot/tgbot.h>
#include <map>
#include <vector>
#include <sqlite3.h>
#include <iostream>
#include <thread>
#include <chrono>
#include <ctime>
#include <cstdlib>
#include <sstream>
#include <algorithm>
#include <iomanip>
#include <cpr/cpr.h>
#include <nlohmann/json.hpp>
#include <windows.h>
#include <io.h>
#include <fcntl.h>
#include <string>

std::string toLowerASCII(const std::string& str) {
    std::string result = str;
    std::transform(result.begin(), result.end(), result.begin(),
        [](unsigned char c) -> char {
            if (c >= 'A' && c <= 'Z') return c + 32;
            return static_cast<char>(c);
        });
    return result;
}

struct Product {
    int id;
    std::string name;
    std::string description;
    std::string brand;
    std::string image_url;
    std::string sku;
    double price;
    double discount_price;
    std::string category_name;
};


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


std::vector<Product> findCompatibleProducts(const Product& target, const std::vector<Product>& allProducts) {
    std::vector<Product> matches;

    std::string targetBrandLower = toLowerASCII(target.brand);
    std::string targetNameLower = toLowerASCII(target.name);
    std::string targetDescLower = toLowerASCII(target.description.substr(0, 10));

    for (const auto& p : allProducts) {
        if (p.id == target.id) continue;

        std::string combined = p.name + " " + p.description + " " + p.brand;
        std::string combinedLower = toLowerASCII(combined);

        if ((!targetBrandLower.empty() && combinedLower.find(targetBrandLower) != std::string::npos) ||
            (!targetNameLower.empty() && combinedLower.find(targetNameLower) != std::string::npos) ||
            (!targetDescLower.empty() && combinedLower.find(targetDescLower) != std::string::npos)) {
            matches.push_back(p);
        }
    }
    return matches;
}

std::map<std::string, std::vector<std::string>> compatibleCategoriesMap = {
    {"Футболки", {"Брюки", "Кроссовки"}},
    {"Рубашки", {"Брюки", "Туфли"}},
    {"Платья", {"Блузки", "Туфли"}},
    {"Куртки", {"Джинсы", "Ботинки"}},
    {"Брюки", {"Футболки", "Кроссовки"}},
    {"Кроссовки", {"Футболки", "Брюки"}}
};

std::vector<Product> findCompatibleByCategory(const Product& targetProduct, const std::vector<Product>& allProducts) {
    std::vector<Product> compatibleProducts;

    auto it = compatibleCategoriesMap.find(targetProduct.category_name);
    if (it == compatibleCategoriesMap.end()) {
        return compatibleProducts; 
    }

    const std::vector<std::string>& compatibleCats = it->second;


    for (const auto& product : allProducts) {
        if (std::find(compatibleCats.begin(), compatibleCats.end(), product.category_name) != compatibleCats.end() &&
            product.id != targetProduct.id) {
            compatibleProducts.push_back(product);
        }
    }

    return compatibleProducts;
}

int main() {
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);


    TgBot::Bot bot("7819743495:AAH8poZ9bSwTQC7KGF5y3yXqfvdr5Zgy0Co");

    try {
        bot.getApi().deleteWebhook();
        std::cout << "Webhook deleted successfully." << std::endl;
    }
    catch (TgBot::TgException& e) {
        std::cerr << "Failed to delete webhook: " << e.what() << std::endl;
        return 1;
    }

    struct UserData {
        int step = 0;
        std::map<char, int> answers;
        int lastQuestionMessageId = 0;
        bool awaitingProductId = false;
    };

    struct Question {
        std::string text;
        std::vector<std::string> options;
    };

    std::vector<Question> questions = {
        {u8"1. Какие цвета преобладают в вашей одежде?", {u8"A. Черный, белый, серый, бежевый, синий", u8"B. Удобные, любые цвета", u8"C. Пастельные и светлые", u8"D. Яркие, контрастные"}},
        {u8"2. Что для вас важнее всего при выборе одежды?", {u8"A. Элегантность и соответствие случаю", u8"B. Комфорт и свобода движений", u8"C. Нежность и легкость", u8"D. Привлечение внимания и выражение индивидуальности"}},
        {u8"3. Какую обувь вы выберете для повседневной носки?", {u8"A. Классические туфли или ботинки", u8"B. Кроссовки или кеды", u8"C. Балетки или сандалии", u8"D. Необычную обувь с ярким дизайном"}},
        {u8"4. Какие ткани вы предпочитаете?", {u8"A. Плотные, хорошо держащие форму", u8"B. Мягкие, удобные, спортивные", u8"C. Легкие, воздушные", u8"D. Необычные, текстурные"}},
        {u8"5. Какие аксессуары вы считаете важными?", {u8"A. Минималистичные, качественные", u8"B. Удобные, функциональные", u8"C. Нежные, изящные", u8"D. Эффектные, привлекающие внимание"}},
        {u8"6. Какой крой одежды вам ближе?", {u8"A. Прямой, строгий", u8"B. Свободный, спортивный", u8"C. Плавный, приталенный", u8"D. Необычный, асимметричный"}},
        {u8"7. Какую верхнюю одежду вы обычно выбираете?", {u8"A. Классическое пальто или тренч", u8"B. Куртку-бомбер или спортивную парку", u8"C. Легкий плащ или кардиган", u8"D. Яркое, необычное пальто или куртку"}},
        {u8"8. Как вы относитесь к деталям в одежде?", {u8"A. Минимум деталей", u8"B. Функциональные детали", u8"C. Декоративные детали (рюши, кружево)", u8"D. Необычные, привлекающие внимание детали"}},
        {u8"9. Какую одежду вы выберете для вечеринки?", {u8"A. Элегантное платье или костюм", u8"B. Спортивный костюм или удобный наряд", u8"C. Легкое платье или блузку с юбкой", u8"D. Яркий, экстравагантный наряд"}},
        {u8"10. Что для вас самое важное в стиле?", {u8"A. Вневременная элегантность", u8"B. Комфорт и функциональность", u8"C. Нежность и романтика", u8"D. Выражение индивидуальности и креативности"}}
    };

    std::vector<std::string> classicStyleTips = {
        u8"👔 Совет дня: Носите однотонные рубашки с костюмом для строгого образа.",
        u8"💼 Совет дня: Инвестируйте в качественный кожаный портфель.",
        u8"⌚️ Совет дня: Выбирайте классические часы с кожаным ремешком."
    };

    std::vector<std::string> sportStyleTips = {
        u8"👟 Совет дня: Сочетайте спортивные штаны с футболкой оверсайз для удобного образа.",
        u8"🧢 Совет дня: Носите бейсболку для защиты от солнца и стильного вида.",
        u8"🎒 Совет дня: Выбирайте рюкзак с удобными лямками для активного образа жизни."
    };

    std::vector<std::string> romanticStyleTips = {
        u8"🌸 Совет дня: Носите платья с цветочным принтом для романтичного образа.",
        u8"🎀 Совет дня: Дополните образ бантом в волосах или на одежде.",
        u8"👡 Совет дня: Выбирайте балетки или сандалии на плоской подошве."
    };


    std::vector<std::string> dramaticStyleTips = {
        u8"🎭 Совет дня: Носите одежду с необычными вырезами и асимметрией.",
        u8"💄 Совет дня: Подчеркните губы яркой помадой для эффектного образа.",
        u8"💍 Совет дня: Выбирайте крупные, броские украшения."
    };

    auto getMainMenuKeyboard = []() -> TgBot::InlineKeyboardMarkup::Ptr {
        TgBot::InlineKeyboardMarkup::Ptr keyboard(new TgBot::InlineKeyboardMarkup);

        std::vector<TgBot::InlineKeyboardButton::Ptr> row1;
        TgBot::InlineKeyboardButton::Ptr button1(new TgBot::InlineKeyboardButton);
        button1->text = u8"🔍 Поиск товара по артикулу";
        button1->callbackData = "search_product";
        row1.push_back(button1);

        std::vector<TgBot::InlineKeyboardButton::Ptr> row2;
        TgBot::InlineKeyboardButton::Ptr button2(new TgBot::InlineKeyboardButton);
        button2->text = u8"🎨 Тест на определение твоего стиля";
        button2->callbackData = "start_test";
        row2.push_back(button2);

        std::vector<TgBot::InlineKeyboardButton::Ptr> row3;
        TgBot::InlineKeyboardButton::Ptr button3(new TgBot::InlineKeyboardButton);
        button3->text = u8"⚙️ Настройки";
        button3->callbackData = "settings";
        row3.push_back(button3);


        keyboard->inlineKeyboard.push_back(row1);
        keyboard->inlineKeyboard.push_back(row2);
        keyboard->inlineKeyboard.push_back(row3);

        return keyboard;
        };


    auto getAnswerButtons = [&getMainMenuKeyboard](bool allowBack) -> TgBot::InlineKeyboardMarkup::Ptr {
        TgBot::InlineKeyboardMarkup::Ptr keyboard(new TgBot::InlineKeyboardMarkup);
        std::vector<TgBot::InlineKeyboardButton::Ptr> row1, row2;

        for (char ch = 'A'; ch <= 'D'; ++ch) {
            TgBot::InlineKeyboardButton::Ptr btn(new TgBot::InlineKeyboardButton);
            btn->text = std::string(1, ch);
            btn->callbackData = std::string("answer_") + ch;
            if (ch == 'A' || ch == 'B') row1.push_back(btn);
            else row2.push_back(btn);
        }

        if (!row1.empty()) {
            keyboard->inlineKeyboard.push_back(row1);
        }
        if (!row2.empty()) {
            keyboard->inlineKeyboard.push_back(row2);
        }


        std::vector<TgBot::InlineKeyboardButton::Ptr> rowBack;
        if (allowBack) {
            TgBot::InlineKeyboardButton::Ptr backBtn(new TgBot::InlineKeyboardButton);
            backBtn->text = u8"⬅️ Назад";
            backBtn->callbackData = "back";
            rowBack.push_back(backBtn);
            if (!rowBack.empty()) {
                keyboard->inlineKeyboard.push_back(rowBack);
            }


        }


        std::vector<TgBot::InlineKeyboardButton::Ptr> rowMenu;
        TgBot::InlineKeyboardButton::Ptr menuBtn(new TgBot::InlineKeyboardButton);
        menuBtn->text = u8"🏠 Главное меню";
        menuBtn->callbackData = "main_menu";
        rowMenu.push_back(menuBtn);
        if (!rowMenu.empty()) {
            keyboard->inlineKeyboard.push_back(rowMenu);
        }




        return keyboard;
        };

    sqlite3* db;
    int rc = sqlite3_open("bot_users.db", &db);

    if (rc) {
        std::cerr << "Can't open database: " << sqlite3_errmsg(db) << std::endl;
        return 1;
    }
    else {
        std::cout << "Opened database successfully" << std::endl;
    }

    const char* sql_create_table = "CREATE TABLE IF NOT EXISTS users ("
        "id INTEGER PRIMARY KEY, "
        "chat_id INTEGER UNIQUE NOT NULL, "
        "daily_tips_enabled INTEGER DEFAULT 1, "
        "style CHAR(1));";

    char* zErrMsg = 0;
    rc = sqlite3_exec(db, sql_create_table, 0, 0, &zErrMsg);

    if (rc != SQLITE_OK) {
        std::cerr << "SQL error: " << zErrMsg << std::endl;
        sqlite3_free(zErrMsg);
    }
    else {
        std::cout << "Table created successfully" << std::endl;
    }


    auto addUserToDatabase = [&db](int64_t chat_id) {
        std::string sql_insert = "INSERT OR IGNORE INTO users (chat_id) VALUES (" + std::to_string(chat_id) + ");";
        char* zErrMsg = 0;
        int rc = sqlite3_exec(db, sql_insert.c_str(), 0, 0, &zErrMsg);

        if (rc != SQLITE_OK) {
            std::cerr << "SQL error: " << zErrMsg << std::endl;
            sqlite3_free(zErrMsg);
        }
        else {
            std::cout << "User with chat_id " << chat_id << " added (or already exists) to database." << std::endl;
        }
        };


    auto setUserStyle = [&db](int64_t chat_id, char style) {
        std::string sql_update = "UPDATE users SET style = '" + std::string(1, style) +
            "' WHERE chat_id = " + std::to_string(chat_id) + ";";
        char* zErrMsg = 0;
        int rc = sqlite3_exec(db, sql_update.c_str(), 0, 0, &zErrMsg);

        if (rc != SQLITE_OK) {
            std::cerr << "SQL error: " << zErrMsg << std::endl;
            sqlite3_free(zErrMsg);
        }
        else {
            std::cout << "Style for chat_id " << chat_id << " set to " << style << std::endl;
        }
        };

    auto getStyleTips = [&](char style) -> std::vector<std::string>&{
        switch (style) {
        case 'A': return classicStyleTips;
        case 'B': return sportStyleTips;
        case 'C': return romanticStyleTips;
        case 'D': return dramaticStyleTips;
        default: return classicStyleTips;
        }
        };



    auto isDailyTipsEnabled = [&db](int64_t chat_id) -> bool {
        std::string sql = "SELECT daily_tips_enabled FROM users WHERE chat_id = " + std::to_string(chat_id) + ";";
        sqlite3_stmt* stmt;
        bool enabled = true;

        if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, NULL) == SQLITE_OK) {
            if (sqlite3_step(stmt) == SQLITE_ROW) {
                enabled = sqlite3_column_int(stmt, 0) == 1;
            }
            sqlite3_finalize(stmt);
        }
        else {
            std::cerr << "Failed to prepare select statement\n";
        }
        return enabled;
        };

    auto setDailyTipsEnabled = [&db](int64_t chat_id, bool enabled) {
        std::string sql = "UPDATE users SET daily_tips_enabled = " + std::to_string(enabled ? 1 : 0) +
            " WHERE chat_id = " + std::to_string(chat_id) + ";";

        char* zErrMsg = 0;
        int rc = sqlite3_exec(db, sql.c_str(), 0, 0, &zErrMsg);

        if (rc != SQLITE_OK) {
            std::cerr << "SQL error: " << zErrMsg << std::endl;
            sqlite3_free(zErrMsg);
        }
        else {
            std::cout << "Daily tips for chat_id " << chat_id << " set to " << enabled << std::endl;
        }
        };


    std::map<int64_t, UserData> users;
    std::map<int64_t, int> previousQuestionMessageIds;

    bot.getEvents().onCommand("start", [&bot, &addUserToDatabase, &getMainMenuKeyboard](TgBot::Message::Ptr message) {
        int64_t chat_id = message->chat->id;
        addUserToDatabase(chat_id);

        bot.getApi().sendMessage(chat_id, u8"✨ Привет! Я твой личный помощник по стилю.\n\nЧто ты хочешь сделать?", false, 0, getMainMenuKeyboard());
        });

    bot.getEvents().onCallbackQuery([&bot, &users, &questions, &getAnswerButtons, &isDailyTipsEnabled, &setDailyTipsEnabled, &setUserStyle, &previousQuestionMessageIds, &getMainMenuKeyboard](TgBot::CallbackQuery::Ptr query) {
        int64_t chatId = query->message->chat->id;
        int messageId = query->message->messageId;
        std::string data = query->data;

        if (data == "main_menu") {
            bot.getApi().editMessageText(u8"✨ Привет! Я твой личный помощник по стилю.\n\nЧто ты хочешь сделать?", chatId, messageId, "", "Markdown", false, getMainMenuKeyboard());
            if (users.count(chatId)) {
                users.erase(chatId); 
            }

            return;
        }


        if (data == "search_product") {
            users[chatId].awaitingProductId = true;
            bot.getApi().sendMessage(chatId, u8"📦 Введите ID товара для поиска:");
        }

        if (data == "start_test") {
            users[chatId] = UserData();
            bot.getApi().sendMessage(chatId, u8"🎯 Начинаем тест! Отвечай на вопросы, нажимая кнопки.");
            const auto& q = questions[0];
            TgBot::Message::Ptr sentMessage = bot.getApi().sendMessage(chatId, q.text + u8"\n\n" +
                q.options[0] + "\n" + q.options[1] + "\n" + q.options[2] + "\n" + q.options[3],
                false, 0, getAnswerButtons(false));
            users[chatId].lastQuestionMessageId = sentMessage->messageId;
            previousQuestionMessageIds[chatId] = sentMessage->messageId;
        }

        if (data == "back") {
            if (users.find(chatId) != users.end() && users[chatId].step > 0) {
                users[chatId].step--;
                const auto& q = questions[users[chatId].step];

                try {
                    bot.getApi().deleteMessage(chatId, previousQuestionMessageIds[chatId]);
                }
                catch (TgBot::TgException& e) {
                    std::cerr << "Failed to delete message: " << e.what() << std::endl;
                }

                TgBot::Message::Ptr sentMessage = bot.getApi().sendMessage(chatId, q.text + u8"\n\n" +
                    q.options[0] + "\n" + q.options[1] + "\n" + q.options[2] + "\n" + q.options[3],
                    false, 0, getAnswerButtons(users[chatId].step > 0));
                users[chatId].lastQuestionMessageId = sentMessage->messageId;
                previousQuestionMessageIds[chatId] = sentMessage->messageId;
            }
            else {
                bot.getApi().answerCallbackQuery(query->id, u8"Некуда возвращаться.", true);
            }
        }

        if (data.rfind("answer_", 0) == 0) {
            if (users.find(chatId) != users.end() && users[chatId].lastQuestionMessageId == messageId) {
                char answer = data[7];
                users[chatId].answers[answer]++;
                users[chatId].step++;
                users[chatId].lastQuestionMessageId = 0;

                try {
                    bot.getApi().deleteMessage(chatId, previousQuestionMessageIds[chatId]);
                }
                catch (TgBot::TgException& e) {
                    std::cerr << "Failed to delete message: " << e.what() << std::endl;
                }

                if (users[chatId].step >= questions.size()) {
                    char result = 'A';
                    int maxCount = 0;
                    for (auto& [key, count] : users[chatId].answers) {
                        if (count > maxCount) {
                            maxCount = count;
                            result = key;
                        }
                    }


                    std::string styleResult;
                    std::string styleDescription;
                    switch (result) {
                    case 'A':
                        styleResult = u8"Классический стиль";
                        styleDescription = u8"Строгие линии, сдержанные цвета (чёрный, белый, серый, бежевый, синий), минимализм в деталях. Костюмы, рубашки, пальто прямого кроя — всё выглядит элегантно и вне времени.";
                        break;
                    case 'B':
                        styleResult = u8"Спортивный стиль";
                        styleDescription = u8"Удобная, функциональная одежда: худи, кроссовки, спортивные костюмы, футболки. Всё про комфорт и свободу движения. Сейчас часто сочетается с элементами уличной моды.";
                        break;
                    case 'C':
                        styleResult = u8"Романтический стиль";
                        styleDescription = u8"Лёгкие, нежные ткани (шифон, кружево), пастельные тона, цветочные принты, плавные линии. Часто включает платья, блузы с рюшами, юбки.";
                        break;
                    case 'D':
                        styleResult = u8"Драматический стиль";
                        styleDescription = u8"Яркие цвета, сложные формы, привлекающие внимание детали. Чёткие контрасты, нестандартные решения, дизайнерские вещи, акцент на эффектность образа.";
                        break;
                    }

                    int totalAnswers = questions.size();
                    std::map<char, double> stylePercentages;
                    stylePercentages['A'] = (double)users[chatId].answers['A'] / totalAnswers * 100.0;
                    stylePercentages['B'] = (double)users[chatId].answers['B'] / totalAnswers * 100.0;
                    stylePercentages['C'] = (double)users[chatId].answers['C'] / totalAnswers * 100.0;
                    stylePercentages['D'] = (double)users[chatId].answers['D'] / totalAnswers * 100.0;

                    std::stringstream ss;
                    ss << u8"✅ Готово!\n\n" << styleResult << u8"\n\n" << styleDescription << "\n\n";
                    ss << u8"📊 Распределение по стилям:\n";
                    ss << u8"Классический (A): " << std::fixed << std::setprecision(1) << stylePercentages['A'] << "%\n";
                    ss << u8"Спортивный (B): " << std::fixed << std::setprecision(1) << stylePercentages['B'] << "%\n";
                    ss << u8"Романтический (C): " << std::fixed << std::setprecision(1) << stylePercentages['C'] << "%\n";
                    ss << u8"Драматический (D): " << std::fixed << std::setprecision(1) << stylePercentages['D'] << "%\n";

                    bot.getApi().sendMessage(chatId, ss.str(), false, 0, getMainMenuKeyboard());
                    setUserStyle(chatId, result);
                    users.erase(chatId);
                    previousQuestionMessageIds.erase(chatId);
                }
                else {
                    const auto& q = questions[users[chatId].step];
                    TgBot::Message::Ptr sentMessage = bot.getApi().sendMessage(chatId, q.text + u8"\n\n" +
                        q.options[0] + "\n" + q.options[1] + "\n" + q.options[2] + "\n" + q.options[3],
                        false, 0, getAnswerButtons(users[chatId].step > 0));
                    users[chatId].lastQuestionMessageId = sentMessage->messageId;
                    previousQuestionMessageIds[chatId] = sentMessage->messageId;
                }
            }
            else {
                bot.getApi().answerCallbackQuery(query->id, u8"Пожалуйста, ответьте на последний заданный вопрос.", true);
            }
        }

        if (data == "settings") {
            TgBot::InlineKeyboardMarkup::Ptr keyboard(new TgBot::InlineKeyboardMarkup);


            std::vector<TgBot::InlineKeyboardButton::Ptr> rowMenu;
            TgBot::InlineKeyboardButton::Ptr menuBtn(new TgBot::InlineKeyboardButton);
            menuBtn->text = u8"🏠 Главное меню";
            menuBtn->callbackData = "main_menu";
            rowMenu.push_back(menuBtn);



            std::vector<TgBot::InlineKeyboardButton::Ptr> row1;

            TgBot::InlineKeyboardButton::Ptr dailyTipsButton(new TgBot::InlineKeyboardButton);
            bool tipsEnabled = isDailyTipsEnabled(chatId);
            dailyTipsButton->text = (tipsEnabled ? u8"🚫 Выключить советы по стилю" : u8"✅ Включить советы по стилю");
            dailyTipsButton->callbackData = "toggle_daily_tips";
            row1.push_back(dailyTipsButton);




            keyboard->inlineKeyboard.push_back(row1);
            keyboard->inlineKeyboard.push_back(rowMenu);

            bot.getApi().editMessageText(u8"⚙️ Настройки:\n\nЗдесь ты можешь настроить параметры бота.", chatId, messageId, "", "Markdown", false, keyboard);

        }

        if (data == "toggle_daily_tips") {
            bool currentSetting = isDailyTipsEnabled(chatId);
            bool newSetting = !currentSetting;

            if (currentSetting != newSetting) {
                setDailyTipsEnabled(chatId, newSetting);

                TgBot::InlineKeyboardMarkup::Ptr keyboard(new TgBot::InlineKeyboardMarkup);

                std::vector<TgBot::InlineKeyboardButton::Ptr> rowMenu;
                TgBot::InlineKeyboardButton::Ptr menuBtn(new TgBot::InlineKeyboardButton);
                menuBtn->text = u8"🏠 Главное меню";
                menuBtn->callbackData = "main_menu";
                rowMenu.push_back(menuBtn);

                std::vector<TgBot::InlineKeyboardButton::Ptr> row1;

                TgBot::InlineKeyboardButton::Ptr dailyTipsButton(new TgBot::InlineKeyboardButton);
                dailyTipsButton->text = (newSetting ? u8"🚫 Выключить советы по стилю" : u8"✅ Включить советы по стилю");
                dailyTipsButton->callbackData = "toggle_daily_tips";
                row1.push_back(dailyTipsButton);



                keyboard->inlineKeyboard.push_back(row1);
                keyboard->inlineKeyboard.push_back(rowMenu);

                bot.getApi().editMessageText(u8"⚙️ Настройки:\n\nЗдесь ты можешь настроить параметры бота.", chatId, query->message->messageId, "", "Markdown", false, keyboard);
            }
            else {
                bot.getApi().answerCallbackQuery(query->id, u8"Состояние уже установлено.", true);
            }
        }

        bot.getApi().answerCallbackQuery(query->id);
        });
    bot.getEvents().onAnyMessage([&bot, &users, &getMainMenuKeyboard](TgBot::Message::Ptr message) {
        if (message->text.empty()) return;

        int64_t chatId = message->chat->id;

        if (users.count(chatId) && users[chatId].awaitingProductId) {
            std::string input = message->text;
            input.erase(std::remove_if(input.begin(), input.end(), ::isspace), input.end());

            if (!std::all_of(input.begin(), input.end(), ::isdigit)) {
                bot.getApi().sendMessage(chatId, u8"❌ Введите корректный номер товара (только цифры).");
                return;
            }

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

                            if (allResp.status_code == 200) {
                                std::string cleanedAll = cleanJson(allResp.text);
                                auto allJson = json::parse(cleanedAll);

                                std::vector<Product> allProducts;
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
                                std::vector<Product> matches = findCompatibleProducts(target, allProducts);

                                if (!matches.empty()) {
                                    bot.getApi().sendMessage(chatId, u8"🧩 Рекомендуемые товары:");
                                    for (const auto& m : matches) {
                                        std::string url = "http://localhost:18080/card?id=" + std::to_string(m.id);
                                        std::string matchMsg = u8"• " + m.name + "\n" + m.description + "\n" + url;


                                        if (!m.image_url.empty()) {
                                            try {
                                                bot.getApi().sendPhoto(chatId, m.image_url, matchMsg);
                                            }
                                            catch (const std::exception& e) {
                                                std::cout << u8"[ERROR] Ошибка при отправке изображения рекомендуемого товара: " << e.what() << std::endl;
                                                bot.getApi().sendMessage(chatId, matchMsg);
                                            }
                                        }
                                        else {
                                            bot.getApi().sendMessage(chatId, matchMsg);
                                        }
                                    }
                                }
                                else {
                                    bot.getApi().sendMessage(chatId, u8"🔎 Рекомендуемых товаров не найдено.");
                                }

                                std::vector<Product> matchesCategory = findCompatibleByCategory(target, allProducts);

                                if (!matchesCategory.empty()) {
                                    bot.getApi().sendMessage(chatId, u8"🧩 Совместимые товары по категории:");
                                    for (const auto& m : matchesCategory) {
                                        std::string url = "http://localhost:18080/card?id=" + std::to_string(m.id);
                                        std::string matchMsg = u8"• " + m.name + "\n" + m.description + "\n" + url;

                                        if (!m.image_url.empty()) {
                                            try {
                                                bot.getApi().sendPhoto(chatId, m.image_url, matchMsg);
                                            }
                                            catch (const std::exception& e) {
                                                std::cout << u8"[ERROR] Ошибка при отправке изображения похожего товара: " << e.what() << std::endl;
                                                bot.getApi().sendMessage(chatId, matchMsg);
                                            }
                                        }
                                        else {
                                            bot.getApi().sendMessage(chatId, matchMsg);
                                        }
                                    }
                                }
                                else {
                                    bot.getApi().sendMessage(chatId, u8"🔎 Совместимых товаров по категории не найдено.");
                                }

                            }
                            else {
                                bot.getApi().sendMessage(chatId, u8"⚠️ Не удалось загрузить список всех товаров.");
                            }
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
        else if (!StringTools::startsWith(message->text, "/")) {
            bot.getApi().sendMessage(chatId, u8"Нажми /start, чтобы начать.", false, 0, getMainMenuKeyboard());
        }
        });


    std::thread dailyTipThread([&bot, &db, &classicStyleTips, &sportStyleTips, &romanticStyleTips, &dramaticStyleTips, &getStyleTips]() {
        while (true) {
            std::time_t t = std::time(nullptr) + 3 * 3600;
            std::tm now;
            gmtime_s(&now, &t);

            if (now.tm_hour == 20 && now.tm_min == 17) {
                std::string sql = "SELECT chat_id, style FROM users WHERE daily_tips_enabled = 1 AND style IS NOT NULL;";
                sqlite3_stmt* stmt;
                if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, NULL) == SQLITE_OK) {
                    while (sqlite3_step(stmt) == SQLITE_ROW) {
                        int64_t chatId = sqlite3_column_int64(stmt, 0);
                        const char* styleChar = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));

                        if (styleChar != nullptr && strlen(styleChar) == 1) {
                            char style = styleChar[0];

                            std::vector<std::string>& styleTips = getStyleTips(style);
                            if (!styleTips.empty()) {
                                std::string tip = styleTips[std::rand() % styleTips.size()];
                                try {
                                    bot.getApi().sendMessage(chatId, tip);
                                }
                                catch (const std::exception& e) {
                                    std::cerr << "Failed to send message to " << chatId << ": " << e.what() << std::endl;
                                }
                            }
                            else {
                                std::cerr << "No tips found for style: " << style << std::endl;
                            }
                        }
                        else {
                            std::cerr << "Invalid or missing style for chat_id: " << chatId << std::endl;
                        }
                    }
                    sqlite3_finalize(stmt);
                }
                else {
                    std::cerr << "Failed to prepare select statement\n";
                }
                std::this_thread::sleep_for(std::chrono::seconds(60));
            }
            std::this_thread::sleep_for(std::chrono::seconds(30));
        }
        });

    try {
        printf("Bot username: %s\n", bot.getApi().getMe()->username.c_str());
        TgBot::TgLongPoll longPoll(bot);
        while (true) {
            printf("Long poll started\n");
            longPoll.start();
        }
    }
    catch (TgBot::TgException& e) {
        printf("error: %s\n", e.what());
    }

    dailyTipThread.detach();
    sqlite3_close(db);
    return 0;
}