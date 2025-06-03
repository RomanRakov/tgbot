#include "DailyTipScheduler.h"
#include <thread>
#include <chrono>
#include <ctime>
#include <iostream>
#include <random>

DailyTipScheduler::DailyTipScheduler(TgBot::Bot& bot, sqlite3* db, const StyleService& styleService)
    : bot(bot), db(db), styleService(styleService) {}

void DailyTipScheduler::start() {
    std::thread([this]() { run(); }).detach();
}

void DailyTipScheduler::run() {
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
                            std::random_device rd;
                            std::mt19937 gen(rd());
                            std::uniform_int_distribution<> dis(0, tips.size() - 1);
                            std::string tip = tips[dis(gen)];
                            try {
                                bot.getApi().sendMessage(chatId, tip);
                            }
                            catch (const std::exception& e) {
                                std::cerr << "Error sending tip to " << chatId << ": " << e.what() << std::endl;
                            }
                        }
                    }
                }
                sqlite3_finalize(stmt);
            }
            else {
                std::cerr << "Failed to prepare tip SQL statement." << std::endl;
            }

            std::this_thread::sleep_for(std::chrono::seconds(60)); 
        }

        std::this_thread::sleep_for(std::chrono::seconds(30)); 
    }
}
