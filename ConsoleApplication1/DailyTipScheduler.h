#ifndef DAILY_TIP_SCHEDULER_H
#define DAILY_TIP_SCHEDULER_H

#include <tgbot/tgbot.h>
#include "StyleService.h"
#include "UserService.h"

class DailyTipScheduler {
public:
    DailyTipScheduler(TgBot::Bot& bot, sqlite3* db, const StyleService& styleService);
    void start();  

private:
    TgBot::Bot& bot;
    sqlite3* db;
    const StyleService& styleService;

    void run();
};

#endif 
#pragma once
