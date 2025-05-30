#ifndef PRODUCT_H
#define PRODUCT_H

#include <string>

struct Product {
    int id;
    std::string name;
    std::string description;
    std::string brand;
    std::string image_url;
    std::string sku;
    double price;
    double discount_price;
    std::string category_name;
};

#endif 
