#ifndef PRODUCT_MATCHER_H
#define PRODUCT_MATCHER_H

#include "struct.h"
#include <vector>
#include <map>
#include <string>
#include <set>

class ProductMatcher {
public:
    ProductMatcher();

    std::vector<BotData::Product> findSimilarProducts(
        const BotData::Product& target,
        const std::vector<BotData::Product>& allProducts
    );

    std::vector<BotData::Product> findCompatibleByCategory(
        const BotData::Product& target,
        const std::vector<BotData::Product>& allProducts
    );

private:
    std::map<std::string, std::vector<std::string>> compatibleCategoriesMap;

    static std::string toLowerASCII(const std::string& str);
    static std::set<std::string> tokenize(const std::string& text);
};

#endif // PRODUCT_MATCHER_H
#pragma once
