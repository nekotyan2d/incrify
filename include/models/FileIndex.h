#pragma once
#include <string>
#include <map>

class FileIndex {
    public:
        void set(const std::string& path, const std::string& hash);
        std::string getHash(const std::string& path) const;
        bool hasFile(const std::string& path) const;
        const std::map<std::string, std::string>& entries() const;
    private:
        std::map<std::string, std::string> _entries;
};