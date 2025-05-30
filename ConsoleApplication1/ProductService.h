#ifndef PRODUCT_SERVICE_H
#define PRODUCT_SERVICE_H

#include <string>
#include <vector>
#include <set>
#include "Product.h"

class ProductService {
public:
    static std::string cleanJson(const std::string& raw);

    static std::vector<Product> findCompatibleProductsAdvanced(
        const Product& target,
        const std::vector<Product>& allProducts
    );

    static std::vector<Product> findCompatibleByCategoryFlexible(
        const Product& target,
        const std::vector<Product>& allProducts
    );

private:
    static std::set<std::string> tokenize(const std::string& text);
};

#endif 
