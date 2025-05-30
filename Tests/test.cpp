#include <gtest/gtest.h>

#include "../ConsoleApplication1/ProductService.h"
#include "../ConsoleApplication1/Product.h"
#include "../ConsoleApplication1/QuestionnaireService.h"


TEST(ProductServiceTest, FindByCategoryFlexible) {
    Product target{ 1, "�������� �����", "������� ��������� ��������", "Nike", "", "", 0, 0, "��������" };

    std::vector<Product> all = {
        {2, "����� ������", "����� �����", "Adidas", "", "", 0, 0, "�����"},
        {3, "��������� �����", "���������� �����", "Puma", "", "", 0, 0, "���������"},
        {4, "������", "������� �����", "Zara", "", "", 0, 0, "�������"}
    };

    auto results = ProductService::findCompatibleByCategoryFlexible(target, all);

    ASSERT_EQ(results.size(), 2);
    EXPECT_EQ(results[0].category_name, "�����");
    EXPECT_EQ(results[1].category_name, "���������");
}

TEST(ProductServiceTest, FindByBrandAndDescription) {
    Product target{ 1, "�������� �����", "������, ������", "Nike", "", "", 0, 0, "��������" };

    std::vector<Product> all = {
        {2, "�������� ������", "������ �����, ������", "Nike", "", "", 0, 0, "��������"},
        {3, "�����", "˸����, ������", "Puma", "", "", 0, 0, "�����"},
        {4, "��������", "������� ���������", "Adidas", "", "", 0, 0, "��������"}
    };

    auto results = ProductService::findCompatibleProductsAdvanced(target, all);

    ASSERT_EQ(results.size(), 1);
    EXPECT_EQ(results[0].brand, "Nike");
}


TEST(QuestionnaireServiceTest, CalculateDominantStyle) {
    QuestionnaireService service;
    int64_t chatId = 100;

    service.resetUser(chatId);
    service.recordAnswer(chatId, 'B');
    service.recordAnswer(chatId, 'B');
    service.recordAnswer(chatId, 'A');

    char result = service.calculateResult(chatId);
    EXPECT_EQ(result, 'B');
}


TEST(QuestionnaireServiceTest, AnswerPercentagesAreCorrect) {
    QuestionnaireService service;
    int64_t chatId = 200;

    service.resetUser(chatId);
    service.recordAnswer(chatId, 'A');
    service.recordAnswer(chatId, 'B');
    service.recordAnswer(chatId, 'A');
    service.recordAnswer(chatId, 'C');

    auto percentages = service.getAnswerPercentages(chatId);

    EXPECT_NEAR(percentages['A'], 50.0, 0.01);
    EXPECT_NEAR(percentages['B'], 25.0, 0.01);
    EXPECT_NEAR(percentages['C'], 25.0, 0.01);
}


TEST(QuestionnaireServiceTest, ResetUserClearsProgress) {
    QuestionnaireService service;
    int64_t chatId = 300;

    service.recordAnswer(chatId, 'A');
    service.recordAnswer(chatId, 'B');
    EXPECT_EQ(service.calculateResult(chatId), 'A');

    service.resetUser(chatId);

    service.recordAnswer(chatId, 'C');
    EXPECT_EQ(service.calculateResult(chatId), 'C');
}