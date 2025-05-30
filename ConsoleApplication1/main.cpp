#include <tgbot/tgbot.h>
#include <windows.h>
#include <sqlite3.h>
#include <iostream>

#include "BotHandler.h"
#include "DailyTipScheduler.h"
#include "StyleService.h"

int main() {
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);

    const std::string token = "7819743495:AAH8poZ9bSwTQC7KGF5y3yXqfvdr5Zgy0Co";

    sqlite3* db;
    if (sqlite3_open("bot_users.db", &db) != SQLITE_OK) {
        std::cerr << "Не удалось открыть базу данных: " << sqlite3_errmsg(db) << std::endl;
        return 1;
    }

    const char* createTableSql = R"(
        CREATE TABLE IF NOT EXISTS users (
            id INTEGER PRIMARY KEY,
            chat_id INTEGER UNIQUE NOT NULL,
            daily_tips_enabled INTEGER DEFAULT 1,
            style CHAR(1)
        );
    )";

    char* errMsg = nullptr;
    if (sqlite3_exec(db, createTableSql, nullptr, nullptr, &errMsg) != SQLITE_OK) {
        std::cerr << "Ошибка SQL: " << errMsg << std::endl;
        sqlite3_free(errMsg);
        sqlite3_close(db);
        return 1;
    }

    TgBot::Bot bot(token);
    StyleService styleService;
    DailyTipScheduler tipScheduler(bot, db, styleService);
    tipScheduler.start();

    BotHandler handler(token, db);
    handler.start();

    sqlite3_close(db);
    return 0;
}
