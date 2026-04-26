#pragma once
#include <string>
#include <vector>
#include <memory>
#include "FileIndexer.h"
#include "IProgressObserver.h"
#include "StorageEngine.h"

struct Result {
    bool success;
    std::string message;
};

class BackupManager {
    public:
        explicit BackupManager(std::unique_ptr<FileIndexer> indexer);

        void addObserver(IProgressObserver* observer);
        
        Result createFullBackup(const std::string& src, const std::string& dst);
        Result createIncremental(const std::string& src, const std::string& dst);

        Result restore(const std::string& snapshotId, const std::string& backupDir);
        Result deleteSnapshot(const std::string& snapshotId, const std::string& backupDir);
        std::vector<Snapshot> listSnapshots(const std::string& backupDir);

    private:
        std::unique_ptr<FileIndexer> _indexer;
        std::vector<IProgressObserver*> _observers;
        
        void notify(const std::string& message);
        std::string generateId() const;
};