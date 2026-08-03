// TextRPGSource/ShopLayout.h
#pragma once

#include <fstream>
#include <string>

struct ShopLayout
{
    int outputPixelWidth = 1000;
    int characterHeightScale = 500;
    int contrast = 630;
    float shopkeeperX = 650.0f;
    float shopkeeperY = 86.0f;
    float shopkeeperWidth = 614.0f;
    float shopkeeperHeight = 842.0f;
};

inline ShopLayout LoadShopLayout(const char* fileName = "ShopLayout.jsonc")
{
    ShopLayout layout;
    std::ifstream file(fileName);
    std::string line;
    while (std::getline(file, line))
    {
        const auto equals = line.find('=');
        if (equals == std::string::npos) continue;
        const std::string key = line.substr(0, equals);
        const std::string value = line.substr(equals + 1);
        try
        {
            if (key == "output_pixel_width") layout.outputPixelWidth = std::stoi(value);
            else if (key == "character_height_scale") layout.characterHeightScale = std::stoi(value);
            else if (key == "contrast") layout.contrast = std::stoi(value);
            else if (key == "shopkeeper_x") layout.shopkeeperX = std::stof(value);
            else if (key == "shopkeeper_y") layout.shopkeeperY = std::stof(value);
            else if (key == "shopkeeper_width") layout.shopkeeperWidth = std::stof(value);
            else if (key == "shopkeeper_height") layout.shopkeeperHeight = std::stof(value);
        }
        catch (...) {}
    }
    return layout;
}

inline void SaveShopLayout(const ShopLayout& layout, const char* fileName = "ShopLayout.jsonc")
{
    std::ofstream file(fileName, std::ios::trunc);
    file << "# Shop AA layout settings\n";
    file << "# P: placement mode, drag: move shopkeeper, +/-: resize, S: save, X: cancel\n";
    file << "output_pixel_width=" << layout.outputPixelWidth << "\n";
    file << "character_height_scale=" << layout.characterHeightScale << "\n";
    file << "contrast=" << layout.contrast << "\n";
    file << "shopkeeper_x=" << layout.shopkeeperX << "\n";
    file << "shopkeeper_y=" << layout.shopkeeperY << "\n";
    file << "shopkeeper_width=" << layout.shopkeeperWidth << "\n";
    file << "shopkeeper_height=" << layout.shopkeeperHeight << "\n";
}
