#include "models/FileIndex.h"

void FileIndex::set(const std::string& path, const std::string& hash){
    _entries[path] = hash;
}

std::string FileIndex::getHash(const std::string& path) const {
    auto it = _entries.find(path);
    return it != _entries.end() ? it->second : "";
}

bool FileIndex::hasFile(const std::string& path) const {
    return _entries.find(path) != _entries.end();
}

const std::map<std::string, std::string>& FileIndex::entries() const {
    return _entries;
}