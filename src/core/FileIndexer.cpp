#include "core/FileIndexer.h"

namespace fs = std::filesystem;

FileIndexer::FileIndexer(std::unique_ptr<IHashStrategy> strategy) : _strategy(std::move(strategy)) {}

FileIndex FileIndexer::buildIndex(const std::string& dirPath) const {
    FileIndex index;
    
    fs::path root(dirPath);

    if(!fs::exists(root)){
        throw std::runtime_error("Путь не существует или не является директорией: " + dirPath);
    }

    for(const auto& entry : fs::recursive_directory_iterator(root)){
        if(!entry.is_regular_file()) continue;

        std::string relativePath = fs::relative(entry.path(), root).string();
        std::string hash = _strategy->hash(entry.path().string());
        index.set(relativePath, hash);
    }

    return index;
}

std::vector<FileChange> FileIndexer::compare(const FileIndex& oldIndex, const FileIndex& newIndex) const {
    std::vector<FileChange> changes;

    for(const auto& [path, hash] : newIndex.entries()){
        if(!oldIndex.hasFile(path)){
            changes.push_back({path, ChangeStatus::ADDED, hash});
        } else if(oldIndex.getHash(path) != hash){
            changes.push_back({path, ChangeStatus::MODIFIED, hash});
        }
    }

    for (const auto& [path, hash] : oldIndex.entries()){
        if(!newIndex.hasFile(path)){
            changes.push_back({path, ChangeStatus::DELETED, ""});
        }
    }

    return changes;
}