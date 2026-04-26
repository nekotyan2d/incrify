#include "core/CRC32Strategy.h"
#include <fstream>
#include <vector>
#include <sstream>
#include <iomanip>
#include <stdexcept>

uint32_t CRC32Strategy::computeCRC32(const uint8_t* data, size_t length) const {
    uint32_t crc = 0xFFFFFFFF;
    for (size_t i = 0; i < length; i++) {
        crc ^= data[i];
        for (int j = 0; j < 8; j++) {
            crc = (crc & 1) ? (crc >> 1) ^ 0xEDB88320 : (crc >> 1);
        }
    }
    return ~crc;
}

std::string CRC32Strategy::hash(const std::string& filePath) const {
    std::ifstream file(filePath, std::ios::binary);
    if (!file.is_open())
        throw std::runtime_error("Не удалось открыть файл: " + filePath);
 
    uint32_t crc = 0xFFFFFFFF;
    std::vector<uint8_t> buffer(8192);
 
    while (file.read(reinterpret_cast<char*>(buffer.data()), buffer.size()) || file.gcount() > 0) {
        size_t count = static_cast<size_t>(file.gcount());
        for (size_t i = 0; i < count; i++) {
            crc ^= buffer[i];
            for (int j = 0; j < 8; j++) {
                crc = (crc & 1) ? (crc >> 1) ^ 0xEDB88320 : (crc >> 1);
            }
        }
    }
 
    crc = ~crc;
    std::ostringstream oss;
    oss << std::hex << std::setw(8) << std::setfill('0') << crc;
    return oss.str();
}