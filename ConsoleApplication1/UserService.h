#ifndef USER_SERVICE_H
#define USER_SERVICE_H

#include <sqlite3.h>
#include <cstdint>
#include <string>

class UserService {
public:
    explicit UserService(sqlite3* database);

    void addUser(int64_t chat_id);
    void setUserStyle(int64_t chat_id, char style);
    bool isDailyTipsEnabled(int64_t chat_id);
    void setDailyTipsEnabled(int64_t chat_id, bool enabled);

private:
    sqlite3* db;
};

#endif 
#pragma once
