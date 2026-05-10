#pragma once
#include "IHashStrategy.h"
#include <cstdint>
#include <fstream>
#include <vector>
#include <sstream>
#include <iomanip>
#include <stdexcept>

class CRC32Strategy : public IHashStrategy {
    public:
        std::string hash(const std::string& filePath) const override;

    private:
        uint32_t computeCRC32(const uint8_t* data, size_t length) const;
};