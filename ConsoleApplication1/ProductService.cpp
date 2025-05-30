#include "ProductService.h"
#include <sstream>
#include <algorithm>
#include <map>

static std::string toLowerASCII(const std::string& str) {
    std::string result = str;
    std::transform(result.begin(), result.end(), result.begin(),
        [](unsigned char c) -> char {
            if (c >= 'A' && c <= 'Z') return c + 32;
            return static_cast<char>(c);
        });
    return result;
}

std::set<std::string> ProductService::tokenize(const std::string& text) {
    std::istringstream stream(toLowerASCII(text));
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
    std::string targetBrandLower = toLowerASCII(target.brand);

    for (const auto& p : allProducts) {
        if (p.id == target.id) continue;

        int score = 0;

        if (toLowerASCII(p.brand) == targetBrandLower) score += 3;
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
        {"Футболки", {"Брюки", "Кроссовки"}},
        {"Рубашки", {"Брюки", "Туфли"}},
        {"Платья", {"Блузки", "Туфли"}},
        {"Куртки", {"Джинсы", "Ботинки"}},
        {"Брюки", {"Футболки", "Кроссовки"}},
        {"Кроссовки", {"Футболки", "Брюки"}}
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
