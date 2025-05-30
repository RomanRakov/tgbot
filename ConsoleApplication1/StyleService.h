#ifndef STYLE_SERVICE_H
#define STYLE_SERVICE_H

#include <vector>
#include <string>

class StyleService {
public:
    StyleService();

    const std::vector<std::string>& getTips(char style) const;

private:
    std::vector<std::string> classicTips;
    std::vector<std::string> sportTips;
    std::vector<std::string> romanticTips;
    std::vector<std::string> dramaticTips;
};

#endif 
#pragma once
