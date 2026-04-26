#pragma once
#include <string>

enum class ChangeStatus {
    ADDED,
    MODIFIED,
    DELETED
};

struct FileChange {
    std::string path;
    ChangeStatus status;
    std::string hash;
};