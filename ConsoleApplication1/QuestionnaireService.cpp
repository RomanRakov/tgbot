#include "QuestionnaireService.h"

QuestionnaireService::QuestionnaireService() {
    questions = {
        {u8"1. ����� ����� ����������� � ����� ������?", {
            u8"A. ������, �����, �����, �������, �����",
            u8"B. �������, ����� �����",
            u8"C. ���������� � �������",
            u8"D. �����, �����������"}},

        {u8"2. ��� ��� ��� ������ ����� ��� ������ ������?", {
            u8"A. ������������ � ������������ ������",
            u8"B. ������� � ������� ��������",
            u8"C. �������� � ��������",
            u8"D. ����������� �������� � ��������� ����������������"}},

        {u8"3. ����� ����� �� �������� ��� ������������ �����?", {
            u8"A. ������������ ����� ��� �������",
            u8"B. ��������� ��� ����",
            u8"C. ������� ��� ��������",
            u8"D. ��������� ����� � ����� ��������"}},

        {u8"4. ����� ����� �� �������������?", {
            u8"A. �������, ������ �������� �����",
            u8"B. ������, �������, ����������",
            u8"C. ������, ���������",
            u8"D. ���������, ����������"}},

        {u8"5. ����� ���������� �� �������� �������?", {
            u8"A. ���������������, ������������",
            u8"B. �������, ��������������",
            u8"C. ������, �������",
            u8"D. ���������, ������������ ��������"}},

        {u8"6. ����� ���� ������ ��� �����?", {
            u8"A. ������, �������",
            u8"B. ���������, ����������",
            u8"C. �������, �����������",
            u8"D. ���������, �������������"}},

        {u8"7. ����� ������� ������ �� ������ ���������?", {
            u8"A. ������������ ������ ��� �����",
            u8"B. ������-������ ��� ���������� �����",
            u8"C. ������ ���� ��� ��������",
            u8"D. �����, ��������� ������ ��� ������"}},

        {u8"8. ��� �� ���������� � ������� � ������?", {
            u8"A. ������� �������",
            u8"B. �������������� ������",
            u8"C. ������������ ������ (����, �������)",
            u8"D. ���������, ������������ �������� ������"}},

        {u8"9. ����� ������ �� �������� ��� ���������?", {
            u8"A. ���������� ������ ��� ������",
            u8"B. ���������� ������ ��� ������� �����",
            u8"C. ������ ������ ��� ������ � �����",
            u8"D. �����, ��������������� �����"}},

        {u8"10. ��� ��� ��� ����� ������ � �����?", {
            u8"A. ������������ ������������",
            u8"B. ������� � ����������������",
            u8"C. �������� � ���������",
            u8"D. ��������� ���������������� � ������������"}}
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
