#include "ProductService.h"
#include <sstream>
#include <algorithm>
#include <map>
#include <boost/locale.hpp>

std::string toLowerUnicode(const std::string& str) {
    using namespace boost::locale;
    generator gen;
    std::locale loc = gen("ru_RU.UTF-8");
    return to_lower(str, loc);
}

std::set<std::string> ProductService::tokenize(const std::string& text) {
    std::istringstream stream(toLowerUnicode(text));
    std::set<std::string> tokens;
    std::string word;
    while (stream >> word) {
        if (word.length() > 2) tokens.insert(word);
    }
    return tokens;
}

std::string ProductService::cleanJson(const std::string& raw) {
    auto startPos = raw.find_first_of("{[");
    if (startPos == std::string::npos) {
        return "";
    }

    int depth = 0;
    bool inString = false;
    for (size_t i = startPos; i < raw.size(); ++i) {
        char c = raw[i];
        if (c == '"' && (i == 0 || raw[i - 1] != '\\')) {
            inString = !inString;
        }
        if (!inString) {
            if (c == '{' || c == '[') {
                ++depth;
            }
            else if (c == '}' || c == ']') {
                --depth;
                if (depth == 0) {
                    return raw.substr(startPos, i - startPos + 1);
                }
            }
        }
    }
    return raw.substr(startPos);
}

std::vector<Product> ProductService::findCompatibleProductsAdvanced(
    const Product& target,
    const std::vector<Product>& allProducts
) {
    std::vector<Product> matches;

    std::set<std::string> targetTokens = tokenize(target.name + " " + target.description);
    std::string targetBrandLower = toLowerUnicode(target.brand);

    for (const auto& p : allProducts) {
        if (p.id == target.id) continue;

        int score = 0;

        if (toLowerUnicode(p.brand) == targetBrandLower) score += 3;
        if (p.category_name == target.category_name) score += 2;

        std::set<std::string> productTokens = tokenize(p.name + " " + p.description);
        int commonWords = std::count_if(
            targetTokens.begin(), targetTokens.end(),
            [&](const std::string& word) { return productTokens.count(word) > 0; }
        );
        score += commonWords;

        if (score >= 4) {
            matches.push_back(p);
        }
    }

    return matches;
}

std::vector<Product> ProductService::findCompatibleByCategoryFlexible(
    const Product& target,
    const std::vector<Product>& allProducts
) {
    static const std::map<std::string, std::vector<std::string>> compatibleCategoriesMap = {
        {"Верхняя одежда", {"Повседневная одежда"}},
        {"Повседневная одежда", {"Домашняя одежда", "Обувь"}},
        {"Спортивная и активная одежда", {"Обувь"}},
        {"Офисная / деловая одежда", {"Мужская одежда", "Женская одежда"}},
        {"Вечерняя и торжественная одежда", {"Женская одежда"}},
        {"Мужская одежда", {"Обувь"}},
        {"Женская одежда", {"Обувь"}},
        {"Детская одежда", {"Повседневная одежда", "Обувь"}},
        {"Трендовая одежда", {"Повседневная одежда", "Обувь"}},
        {"Домашняя одежда", {"Обувь"}},
        {"Обувь", {"Домашняя одежда", "Трендовая одежда", "Детская одежда"}}
    };

    std::vector<Product> compatibleProducts;
    auto it = compatibleCategoriesMap.find(target.category_name);
    bool fallbackMode = (it == compatibleCategoriesMap.end());

    std::set<std::string> compatibleCats;
    if (!fallbackMode) {
        compatibleCats.insert(it->second.begin(), it->second.end());
    }

    std::set<std::string> addedCategories;

    for (const auto& product : allProducts) {
        if (product.id == target.id) continue;

        if (!fallbackMode) {
            if (compatibleCats.count(product.category_name)) {
                compatibleProducts.push_back(product);
            }
        }
        else {
            if (product.category_name != target.category_name &&
                addedCategories.count(product.category_name) == 0) {
                compatibleProducts.push_back(product);
                addedCategories.insert(product.category_name);
            }
        }
    }

    return compatibleProducts;
}
