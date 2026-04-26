#pragma once
#include <string>

class IHashStrategy{
    public:
        virtual std::string hash(const std::string& filePath) const = 0;
        virtual ~IHashStrategy() = default;
};