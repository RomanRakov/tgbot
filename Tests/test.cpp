#include <gtest/gtest.h>

#include "../ConsoleApplication1/ProductService.h"
#include "../ConsoleApplication1/Product.h"
#include "../ConsoleApplication1/QuestionnaireService.h"


TEST(ProductServiceTest, CompatibleCategoriesMappedFound) {
    Product target;
    target.id = 1;
    target.category_name = "Верхняя одежда";

    Product match;
    match.id = 2;
    match.category_name = "Повседневная одежда";

    Product other;
    other.id = 3;
    other.category_name = "Обувь";

    std::vector<Product> all = { target, match, other };

    auto result = ProductService::findCompatibleByCategoryFlexible(target, all);

    ASSERT_EQ(result.size(), 1);
    EXPECT_EQ(result[0].id, match.id);
}

TEST(ProductServiceTest, CompatibleCategoriesFallbackUsed) {
    Product target;
    target.id = 1;
    target.category_name = "Неизвестная категория";

    Product p2;
    p2.id = 2;
    p2.category_name = "Обувь";

    Product p3;
    p3.id = 3;
    p3.category_name = "Обувь"; 

    Product p4;
    p4.id = 4;
    p4.category_name = "Повседневная одежда";

    std::vector<Product> all = { target, p2, p3, p4 };

    auto result = ProductService::findCompatibleByCategoryFlexible(target, all);

    ASSERT_EQ(result.size(), 2);
    std::set<int> resultIds = { result[0].id, result[1].id };
    EXPECT_TRUE(resultIds.count(2));
    EXPECT_TRUE(resultIds.count(4));
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
