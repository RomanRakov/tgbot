#include "KeyboardFactory.h"

TgBot::InlineKeyboardMarkup::Ptr KeyboardFactory::getMainMenuKeyboard() {
    TgBot::InlineKeyboardMarkup::Ptr keyboard(new TgBot::InlineKeyboardMarkup);

    auto button1 = TgBot::InlineKeyboardButton::Ptr(new TgBot::InlineKeyboardButton);
    button1->text = u8"🔍 Поиск товара по артикулу";
    button1->callbackData = "search_product";

    auto button2 = TgBot::InlineKeyboardButton::Ptr(new TgBot::InlineKeyboardButton);
    button2->text = u8"🎨 Тест на определение твоего стиля";
    button2->callbackData = "start_test";

    auto button3 = TgBot::InlineKeyboardButton::Ptr(new TgBot::InlineKeyboardButton);
    button3->text = u8"⚙️ Настройки";
    button3->callbackData = "settings";

    keyboard->inlineKeyboard.push_back({ button1 });
    keyboard->inlineKeyboard.push_back({ button2 });
    keyboard->inlineKeyboard.push_back({ button3 });

    return keyboard;
}

TgBot::InlineKeyboardMarkup::Ptr KeyboardFactory::getAnswerButtons(bool allowBack) {
    TgBot::InlineKeyboardMarkup::Ptr keyboard(new TgBot::InlineKeyboardMarkup);

    std::vector<TgBot::InlineKeyboardButton::Ptr> row1, row2;
    for (char ch = 'A'; ch <= 'D'; ++ch) {
        auto btn = TgBot::InlineKeyboardButton::Ptr(new TgBot::InlineKeyboardButton);
        btn->text = std::string(1, ch);
        btn->callbackData = "answer_" + std::string(1, ch);
        (ch <= 'B' ? row1 : row2).push_back(btn);
    }

    keyboard->inlineKeyboard.push_back(row1);
    keyboard->inlineKeyboard.push_back(row2);

    if (allowBack) {
        auto backBtn = TgBot::InlineKeyboardButton::Ptr(new TgBot::InlineKeyboardButton);
        backBtn->text = u8"⬅️ Назад";
        backBtn->callbackData = "back";
        keyboard->inlineKeyboard.push_back({ backBtn });
    }

    auto menuBtn = TgBot::InlineKeyboardButton::Ptr(new TgBot::InlineKeyboardButton);
    menuBtn->text = u8"🏠 Главное меню";
    menuBtn->callbackData = "main_menu";
    keyboard->inlineKeyboard.push_back({ menuBtn });

    return keyboard;
}

TgBot::InlineKeyboardMarkup::Ptr KeyboardFactory::getProductMenuKeyboard() {
    TgBot::InlineKeyboardMarkup::Ptr keyboard(new TgBot::InlineKeyboardMarkup);

    auto btnSimilar = TgBot::InlineKeyboardButton::Ptr(new TgBot::InlineKeyboardButton);
    btnSimilar->text = u8"🔄 Похожие товары";
    btnSimilar->callbackData = "similar_products";

    auto btnMatch = TgBot::InlineKeyboardButton::Ptr(new TgBot::InlineKeyboardButton);
    btnMatch->text = u8"👕 С чем сочетать";
    btnMatch->callbackData = "match_products";

    auto btnCare = TgBot::InlineKeyboardButton::Ptr(new TgBot::InlineKeyboardButton);
    btnCare->text = u8"🧼 Как ухаживать";
    btnCare->callbackData = "care_tips";

    auto btnBack = TgBot::InlineKeyboardButton::Ptr(new TgBot::InlineKeyboardButton);
    btnBack->text = u8"⬅️ Назад к поиску";
    btnBack->callbackData = "back_to_search";

    auto btnMenu = TgBot::InlineKeyboardButton::Ptr(new TgBot::InlineKeyboardButton);
    btnMenu->text = u8"🏠 Главное меню";
    btnMenu->callbackData = "main_menu";

    keyboard->inlineKeyboard.push_back({ btnSimilar, btnMatch });
    keyboard->inlineKeyboard.push_back({ btnCare });
    keyboard->inlineKeyboard.push_back({ btnBack, btnMenu });

    return keyboard;
}

TgBot::InlineKeyboardMarkup::Ptr KeyboardFactory::getSettingsKeyboard(bool tipsEnabled) {
    TgBot::InlineKeyboardMarkup::Ptr keyboard(new TgBot::InlineKeyboardMarkup);

    auto dailyTipsButton = TgBot::InlineKeyboardButton::Ptr(new TgBot::InlineKeyboardButton);
    dailyTipsButton->text = tipsEnabled ? u8"🚫 Выключить советы по стилю" : u8"✅ Включить советы по стилю";
    dailyTipsButton->callbackData = "toggle_daily_tips";

    auto menuBtn = TgBot::InlineKeyboardButton::Ptr(new TgBot::InlineKeyboardButton);
    menuBtn->text = u8"🏠 Главное меню";
    menuBtn->callbackData = "main_menu";

    keyboard->inlineKeyboard.push_back({ dailyTipsButton });
    keyboard->inlineKeyboard.push_back({ menuBtn });

    return keyboard;
}
