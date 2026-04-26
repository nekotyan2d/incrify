#pragma once
#include <string>
#include <vector>
#include "FileIndex.h"
#include "FileChange.h"

enum class SnapshotType {
    FULL,
    INCREMENTAL
};

struct Snapshot {
    std::string id; // временная метка
    SnapshotType type;
    std::string parentId; // только для инкрементальных снапшотов
    std::string timestamp;
    std::string srcPath;
    std::string dstPath;
    FileIndex index;
    std::vector<FileChange> changes;
};