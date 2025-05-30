#ifndef KEYBOARD_FACTORY_H
#define KEYBOARD_FACTORY_H

#include <tgbot/tgbot.h>

class KeyboardFactory {
public:
    static TgBot::InlineKeyboardMarkup::Ptr getMainMenuKeyboard();
    static TgBot::InlineKeyboardMarkup::Ptr getAnswerButtons(bool allowBack);
    static TgBot::InlineKeyboardMarkup::Ptr getProductMenuKeyboard();
    static TgBot::InlineKeyboardMarkup::Ptr getSettingsKeyboard(bool tipsEnabled);
};

#endif // KEYBOARD_FACTORY_H
#pragma once
