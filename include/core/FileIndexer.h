#pragma once
#include <memory>
#include <vector>
#include <string>
#include "models/FileIndex.h"
#include "models/FileChange.h"
#include "IHashStrategy.h"
#include <filesystem>
#include <stdexcept>

class FileIndexer {
    public:
        explicit FileIndexer(std::unique_ptr<IHashStrategy> strategy);

        FileIndex buildIndex(const std::string& dirPath) const;
        std::vector<FileChange> compare(const FileIndex& oldIndex, const FileIndex& newIndex) const;

    private:
        std::unique_ptr<IHashStrategy> _strategy;
};