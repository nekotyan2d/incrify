#pragma once
#include <string>
#include <vector>
#include "models/Snapshot.h"
#include "models/FileChange.h"

class StorageEngine {
    public:
        static StorageEngine& getInstance();
        
        StorageEngine(const StorageEngine&) = delete;
        void operator=(const StorageEngine&) = delete;

        void saveSnapshot(const Snapshot& snapshot);
        Snapshot loadSnapshot(const std::string& id, const std::string& dstPath) const;
        Snapshot loadLastSnapshot(const std::string& dstPath) const;

        std::vector<Snapshot> listSnapshots(const std::string& dstPath) const;
        void deleteSnapshot(const std::string& id, const std::string& dstPath);

        void copyFiles(const std::string& srcPath, const std::vector<FileChange>& changes, const std::string& snapshotPath) const;
        void restoreFiles(const std::string& snapshotPath, const std::vector<std::string>& filePaths, const std::string& dstPath) const;

        private:
            StorageEngine() = default;
            std::string getSnapshotsDir(const std::string& dstPath) const;
            std::string getSnapshotPath(const std::string& dstPath, const std::string& id) const;
            std::string getMetaPath(const std::string& dstPath, const std::string&id) const;
};
