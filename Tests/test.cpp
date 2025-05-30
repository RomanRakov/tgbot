#include <gtest/gtest.h>

#include "../ConsoleApplication1/ProductService.h"
#include "../ConsoleApplication1/Product.h"
#include "../ConsoleApplication1/QuestionnaireService.h"


TEST(ProductServiceTest, FindByCategoryFlexible) {
    Product target{ 1, "Футболка белая", "Простая хлопковая футболка", "Nike", "", "", 0, 0, "Футболки" };

    std::vector<Product> all = {
        {2, "Брюки черные", "Узкие брюки", "Adidas", "", "", 0, 0, "Брюки"},
        {3, "Кроссовки белые", "Спортивная обувь", "Puma", "", "", 0, 0, "Кроссовки"},
        {4, "Пиджак", "Офисный стиль", "Zara", "", "", 0, 0, "Пиджаки"}
    };

    auto results = ProductService::findCompatibleByCategoryFlexible(target, all);

    ASSERT_EQ(results.size(), 2);
    EXPECT_EQ(results[0].category_name, "Брюки");
    EXPECT_EQ(results[1].category_name, "Кроссовки");
}

TEST(ProductServiceTest, FindByBrandAndDescription) {
    Product target{ 1, "Футболка белая", "Хлопок, мягкая", "Nike", "", "", 0, 0, "Футболки" };

    std::vector<Product> all = {
        {2, "Футболка черная", "Мягкая ткань, хлопок", "Nike", "", "", 0, 0, "Футболки"},
        {3, "Шорты", "Лёгкие, летние", "Puma", "", "", 0, 0, "Шорты"},
        {4, "Футболка", "Жесткий полиэстер", "Adidas", "", "", 0, 0, "Футболки"}
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