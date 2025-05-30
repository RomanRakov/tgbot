#include "QuestionnaireService.h"

QuestionnaireService::QuestionnaireService() {
    questions = {
        {u8"1. Какие цвета преобладают в вашей одежде?", {
            u8"A. Черный, белый, серый, бежевый, синий",
            u8"B. Удобные, любые цвета",
            u8"C. Пастельные и светлые",
            u8"D. Яркие, контрастные"}},

        {u8"2. Что для вас важнее всего при выборе одежды?", {
            u8"A. Элегантность и соответствие случаю",
            u8"B. Комфорт и свобода движений",
            u8"C. Нежность и легкость",
            u8"D. Привлечение внимания и выражение индивидуальности"}},

        {u8"3. Какую обувь вы выберете для повседневной носки?", {
            u8"A. Классические туфли или ботинки",
            u8"B. Кроссовки или кеды",
            u8"C. Балетки или сандалии",
            u8"D. Необычную обувь с ярким дизайном"}},

        {u8"4. Какие ткани вы предпочитаете?", {
            u8"A. Плотные, хорошо держащие форму",
            u8"B. Мягкие, удобные, спортивные",
            u8"C. Легкие, воздушные",
            u8"D. Необычные, текстурные"}},

        {u8"5. Какие аксессуары вы считаете важными?", {
            u8"A. Минималистичные, качественные",
            u8"B. Удобные, функциональные",
            u8"C. Нежные, изящные",
            u8"D. Эффектные, привлекающие внимание"}},

        {u8"6. Какой крой одежды вам ближе?", {
            u8"A. Прямой, строгий",
            u8"B. Свободный, спортивный",
            u8"C. Плавный, приталенный",
            u8"D. Необычный, асимметричный"}},

        {u8"7. Какую верхнюю одежду вы обычно выбираете?", {
            u8"A. Классическое пальто или тренч",
            u8"B. Куртку-бомбер или спортивную парку",
            u8"C. Легкий плащ или кардиган",
            u8"D. Яркое, необычное пальто или куртку"}},

        {u8"8. Как вы относитесь к деталям в одежде?", {
            u8"A. Минимум деталей",
            u8"B. Функциональные детали",
            u8"C. Декоративные детали (рюши, кружево)",
            u8"D. Необычные, привлекающие внимание детали"}},

        {u8"9. Какую одежду вы выберете для вечеринки?", {
            u8"A. Элегантное платье или костюм",
            u8"B. Спортивный костюм или удобный наряд",
            u8"C. Легкое платье или блузку с юбкой",
            u8"D. Яркий, экстравагантный наряд"}},

        {u8"10. Что для вас самое важное в стиле?", {
            u8"A. Вневременная элегантность",
            u8"B. Комфорт и функциональность",
            u8"C. Нежность и романтика",
            u8"D. Выражение индивидуальности и креативности"}}
    };
}

const Question& QuestionnaireService::getQuestion(size_t index) const {
    return questions.at(index);
}

size_t QuestionnaireService::getTotalQuestions() const {
    return questions.size();
}

void QuestionnaireService::recordAnswer(int64_t chatId, char answer) {
    userProgress[chatId].answers[answer]++;
    userProgress[chatId].step++;
}

char QuestionnaireService::calculateResult(int64_t chatId) const {
    const auto& answers = userProgress.at(chatId).answers;
    char result = 'A';
    int maxCount = 0;

    for (const auto& [style, count] : answers) {
        if (count > maxCount) {
            maxCount = count;
            result = style;
        }
    }

    return result;
}

std::map<char, double> QuestionnaireService::getAnswerPercentages(int64_t chatId) const {
    const auto& answers = userProgress.at(chatId).answers;
    int total = 0;
    for (const auto& [_, count] : answers) {
        total += count;
    }

    std::map<char, double> percentages;
    for (const auto& [style, count] : answers) {
        percentages[style] = (total > 0) ? (count * 100.0 / total) : 0.0;
    }

    return percentages;
}

void QuestionnaireService::resetUser(int64_t chatId) {
    userProgress.erase(chatId);
}
