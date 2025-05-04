#include <stdio.h>
#include <tgbot/tgbot.h>
#include <map>
#include <vector>
#include <sqlite3.h>
#include <iostream> 

int main() {
    TgBot::Bot bot("7819743495:AAH8poZ9bSwTQC7KGF5y3yXqfvdr5Zgy0Co");

    struct UserData {
        int step = 0;
        std::map<char, int> answers;
    };


    struct Question {
        std::string text;
        std::vector<std::string> options;
    };


    std::vector<Question> questions = {
        {u8"1. Что ты наденешь на встречу с друзьями?", {u8"A. Джинсы и футболка", u8"B. Модное по тренду", u8"C. Классика", u8"D. Что-то яркое"}},
        {u8"2. Какой интерьер тебе ближе?", {u8"A. Уютный и минималистичный", u8"B. Современный", u8"C. Стильный и строгий", u8"D. Яркий, необычный"}},
        {u8"3. Как ты ведёшь себя в компании?", {u8"A. Спокойно", u8"B. Общаюсь, но сдержанно", u8"C. Поддерживаю, советую", u8"D. Весёлый и импровизирую"}},
        {u8"4. Что тебе ближе по духу?", {u8"A. Простота и комфорт", u8"B. Элегантность и тренды", u8"C. Статус и утончённость", u8"D. Творчество и оригинальность"}},
        {u8"5. Какой аксессуар выберешь?", {u8"A. Рюкзак", u8"B. Модная сумка", u8"C. Часы", u8"D. Яркие очки"}},
        {u8"6. Какой транспорт тебе ближе?", {u8"A. Велосипед", u8"B. Электросамокат", u8"C. Автомобиль бизнес-класса", u8"D. Мотоцикл или скейт"}},
        {u8"7. Что ты обычно заказываешь в кафе?", {u8"A. Кофе и круассан", u8"B. Авокадо-тост", u8"C. Классический стейк", u8"D. Что-то экзотическое"}},
        {u8"8. Как ты проводишь свободное время?", {u8"A. Дома с книгой или сериалом", u8"B. Прогуливаюсь по городу", u8"C. Посещаю выставки, театры", u8"D. Пробую новые хобби"}},
        {u8"9. Какую обувь выберешь?", {u8"A. Кеды", u8"B. Кроссовки по тренду", u8"C. Кожаные туфли", u8"D. Яркие ботинки или необычные сникеры"}},
        {u8"10. Как ты выбираешь одежду?", {u8"A. Главное — удобно", u8"B. То, что в моде", u8"C. Проверенные классические вещи", u8"D. Что-то необычное и интересное"}}
    };


    auto getAnswerButtons = []() -> TgBot::InlineKeyboardMarkup::Ptr {
        TgBot::InlineKeyboardMarkup::Ptr keyboard(new TgBot::InlineKeyboardMarkup);
        std::vector<TgBot::InlineKeyboardButton::Ptr> row1, row2;

        for (char ch = 'A'; ch <= 'D'; ++ch) {
            TgBot::InlineKeyboardButton::Ptr btn(new TgBot::InlineKeyboardButton);
            btn->text = std::string(1, ch);
            btn->callbackData = std::string("answer_") + ch;
            if (ch == 'A' || ch == 'B') row1.push_back(btn);
            else row2.push_back(btn);
        }
        keyboard->inlineKeyboard.push_back(row1);
        keyboard->inlineKeyboard.push_back(row2);
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
        "chat_id INTEGER UNIQUE NOT NULL);";

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
        std::string sql_insert = "INSERT OR IGNORE INTO users (chat_id) VALUES (" + std::to_string(chat_id) + ");";  // Используем INSERT OR IGNORE

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

        bot.getEvents().onCommand("start", [&bot, &addUserToDatabase](TgBot::Message::Ptr message) {
        int64_t chat_id = message->chat->id;

        addUserToDatabase(chat_id);

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

        keyboard->inlineKeyboard.push_back(row1);
        keyboard->inlineKeyboard.push_back(row2);

        bot.getApi().sendMessage(chat_id, u8"👋 Привет! Я помогу тебе определить стиль.\n\nВыбери, что хочешь сделать:", false, 0, keyboard);
            });


    std::map<int64_t, UserData> users;

    bot.getEvents().onCallbackQuery([&bot, &users, &questions, &getAnswerButtons](TgBot::CallbackQuery::Ptr query) {
        int64_t chatId = query->message->chat->id;
        std::string data = query->data;

        if (data == "search_product") {
            bot.getApi().sendMessage(chatId, u8"🔧 Эта функция пока в разработке.");
        }

        if (data == "start_test") {
            users[chatId] = UserData();
            bot.getApi().sendMessage(chatId, u8"🎯 Начинаем тест! Отвечай на вопросы, нажимая кнопки.");
            const auto& q = questions[0];
            bot.getApi().sendMessage(chatId, q.text + u8"\n\n" +
                q.options[0] + "\n" + q.options[1] + "\n" + q.options[2] + "\n" + q.options[3],
                false, 0, getAnswerButtons());
        }

        if (data.rfind("answer_", 0) == 0) {
            char answer = data[7];
            users[chatId].answers[answer]++;
            users[chatId].step++;

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
                switch (result) {
                case 'A': styleResult = u8"Твой стиль — Кэжуал / Уютный минимализм"; break;
                case 'B': styleResult = u8"Твой стиль — Модный / Современный"; break;
                case 'C': styleResult = u8"Твой стиль — Классический / Элегантный"; break;
                case 'D': styleResult = u8"Твой стиль — Творческий / Экстравагантный"; break;
                }

                bot.getApi().sendMessage(chatId, u8"✅ Готово!\n\n" + styleResult);
                users.erase(chatId);
            }
            else {
                const auto& q = questions[users[chatId].step];
                bot.getApi().sendMessage(chatId, q.text + u8"\n\n" +
                    q.options[0] + "\n" + q.options[1] + "\n" + q.options[2] + "\n" + q.options[3],
                    false, 0, getAnswerButtons());
            }
        }
        bot.getApi().answerCallbackQuery(query->id);
        });

    bot.getEvents().onAnyMessage([&bot](TgBot::Message::Ptr message) {
        if (!StringTools::startsWith(message->text, "/")) {
            bot.getApi().sendMessage(message->chat->id, u8"Нажми /start, чтобы начать.");
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

    sqlite3_close(db);

    return 0;
}