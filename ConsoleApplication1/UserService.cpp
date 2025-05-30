#include "UserService.h"
#include <iostream>

UserService::UserService(sqlite3* database) : db(database) {}

void UserService::addUser(int64_t chat_id) {
    std::string sql_insert = "INSERT OR IGNORE INTO users (chat_id) VALUES (" + std::to_string(chat_id) + ");";
    char* zErrMsg = nullptr;
    int rc = sqlite3_exec(db, sql_insert.c_str(), 0, 0, &zErrMsg);

    if (rc != SQLITE_OK) {
        std::cerr << "SQL error in addUser: " << zErrMsg << std::endl;
        sqlite3_free(zErrMsg);
    }
    else {
        std::cout << "User with chat_id " << chat_id << " added (or already exists) to database." << std::endl;
    }
}

void UserService::setUserStyle(int64_t chat_id, char style) {
    std::string sql_update = "UPDATE users SET style = '" + std::string(1, style) +
        "' WHERE chat_id = " + std::to_string(chat_id) + ";";
    char* zErrMsg = nullptr;
    int rc = sqlite3_exec(db, sql_update.c_str(), 0, 0, &zErrMsg);

    if (rc != SQLITE_OK) {
        std::cerr << "SQL error in setUserStyle: " << zErrMsg << std::endl;
        sqlite3_free(zErrMsg);
    }
    else {
        std::cout << "Style for chat_id " << chat_id << " set to " << style << std::endl;
    }
}

bool UserService::isDailyTipsEnabled(int64_t chat_id) {
    std::string sql = "SELECT daily_tips_enabled FROM users WHERE chat_id = " + std::to_string(chat_id) + ";";
    sqlite3_stmt* stmt = nullptr;
    bool enabled = true;

    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            enabled = sqlite3_column_int(stmt, 0) == 1;
        }
        sqlite3_finalize(stmt);
    }
    else {
        std::cerr << "Failed to prepare select statement in isDailyTipsEnabled\n";
    }

    return enabled;
}

void UserService::setDailyTipsEnabled(int64_t chat_id, bool enabled) {
    std::string sql = "UPDATE users SET daily_tips_enabled = " + std::to_string(enabled ? 1 : 0) +
        " WHERE chat_id = " + std::to_string(chat_id) + ";";

    char* zErrMsg = nullptr;
    int rc = sqlite3_exec(db, sql.c_str(), 0, 0, &zErrMsg);

    if (rc != SQLITE_OK) {
        std::cerr << "SQL error in setDailyTipsEnabled: " << zErrMsg << std::endl;
        sqlite3_free(zErrMsg);
    }
    else {
        std::cout << "Daily tips for chat_id " << chat_id << " set to " << enabled << std::endl;
    }
}
