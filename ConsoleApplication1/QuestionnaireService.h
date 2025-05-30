#ifndef QUESTIONNAIRE_SERVICE_H
#define QUESTIONNAIRE_SERVICE_H

#include <vector>
#include <string>
#include <map>

struct Question {
    std::string text;
    std::vector<std::string> options;
};

class QuestionnaireService {
public:
    QuestionnaireService();

    const Question& getQuestion(size_t index) const;
    size_t getTotalQuestions() const;

    void recordAnswer(int64_t chatId, char answer);
    char calculateResult(int64_t chatId) const;
    std::map<char, double> getAnswerPercentages(int64_t chatId) const;
    void resetUser(int64_t chatId);

private:
    std::vector<Question> questions;

    struct UserProgress {
        int step = 0;
        std::map<char, int> answers;
    };

    std::map<int64_t, UserProgress> userProgress;
};

#endif 
#pragma once
