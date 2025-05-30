#ifndef STRUCTS_H
#define STRUCTS_H

#include <string>
#include <vector>
#include <map>

class BotData {
public:
    struct Product {
        int id = -1;
        std::string name;
        std::string description;
        std::string brand;
        std::string image_url;
        std::string sku;
        double price = 0.0;
        double discount_price = 0.0;
        std::string category_name;
    };

    struct Question {
        std::string text;
        std::vector<std::string> options;
    };

    struct UserData {
        int step = 0;
        std::map<char, int> answers;
        int lastQuestionMessageId = 0;
        bool awaitingProductId = false;
        Product currentProduct;
        std::vector<Product> allProducts;

        void reset() {
            step = 0;
            answers.clear();
            lastQuestionMessageId = 0;
            awaitingProductId = false;
            currentProduct = Product();
            allProducts.clear();
        }
    };
};

#endif // STRUCTS_H
#pragma once
