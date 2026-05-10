#include "core/BackupManager.h"

namespace fs = std::filesystem;

BackupManager::BackupManager(std::unique_ptr<FileIndexer> indexer)
    : _indexer(std::move(indexer)) {}

void BackupManager::addObserver(IProgressObserver* observer) {
    _observers.push_back(observer);
}

void BackupManager::notify(const std::string& message) {
    for (auto* obs : _observers)
        obs->onProgress(message);
}

std::string BackupManager::generateId() const {
    auto now  = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    auto ms   = std::chrono::duration_cast<std::chrono::milliseconds>(
                    now.time_since_epoch()) % 1000;
    std::tm tm{};
#ifdef _WIN32
    localtime_s(&tm, &time);
#else
    localtime_r(&time, &tm);
#endif
    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y%m%d_%H%M%S")
        << "_" << std::setw(3) << std::setfill('0') << ms.count();
    return oss.str();
}

Result BackupManager::createFullBackup(const std::string& src, const std::string& dst) {
    try {
        notify("Построение индекса файлов...");
        FileIndex index = _indexer->buildIndex(src);

        std::vector<FileChange> changes;
        for (const auto& [path, hash] : index.entries())
            changes.push_back({path, ChangeStatus::ADDED, hash});

        Snapshot snapshot;
        snapshot.id = generateId();
        snapshot.type = SnapshotType::FULL;
        snapshot.parentId = "";
        snapshot.timestamp = snapshot.id;
        snapshot.srcPath = fs::absolute(src).string();
        snapshot.dstPath = fs::absolute(dst).string();
        snapshot.index = index;
        snapshot.changes = changes;

        std::string snapshotPath = (fs::path(dst) / ".snapshots" / snapshot.id).string();

        notify("Копирование " + std::to_string(changes.size()) + " файлов...");
        StorageEngine::getInstance().copyFiles(src, changes, snapshotPath);

        notify("Сохранение метаданных...");
        StorageEngine::getInstance().saveSnapshot(snapshot);

        return {true, "Полный бэкап создан: " + snapshot.id +
                      " (" + std::to_string(changes.size()) + " файлов)"};
    } catch (const std::exception& e) {
        return {false, std::string("Ошибка: ") + e.what()};
    }
}

Result BackupManager::createIncremental(const std::string& src, const std::string& dst) {
    try {
        auto& storage = StorageEngine::getInstance();

        notify("Загрузка последнего снапшота...");
        Snapshot lastSnapshot = storage.loadLastSnapshot(fs::absolute(dst).string());

        notify("Построение текущего индекса...");
        FileIndex currentIndex = _indexer->buildIndex(src);

        notify("Сравнение с предыдущим снапшотом...");
        auto changes = _indexer->compare(lastSnapshot.index, currentIndex);

        if (changes.empty())
            return {true, "Изменений не обнаружено — бэкап не нужен."};

        Snapshot snapshot;
        snapshot.id = generateId();
        snapshot.type = SnapshotType::INCREMENTAL;
        snapshot.parentId = lastSnapshot.id;
        snapshot.timestamp = snapshot.id;
        snapshot.srcPath = fs::absolute(src).string();
        snapshot.dstPath = fs::absolute(dst).string();
        snapshot.index = currentIndex;
        snapshot.changes = changes;

        std::string snapshotPath = (fs::path(dst) / ".snapshots" / snapshot.id).string();

        std::vector<FileChange> filesToCopy;
        for (const auto& c : changes)
            if (c.status != ChangeStatus::DELETED)
                filesToCopy.push_back(c);

        notify("Копирование " + std::to_string(filesToCopy.size()) + " изменённых файлов...");
        storage.copyFiles(src, filesToCopy, snapshotPath);

        notify("Сохранение метаданных...");
        storage.saveSnapshot(snapshot);

        int added = 0, modified = 0, deleted = 0;
        for (const auto& c : changes) {
            if      (c.status == ChangeStatus::ADDED)    added++;
            else if (c.status == ChangeStatus::MODIFIED) modified++;
            else                                          deleted++;
        }

        return {true, "Инкрементальный бэкап создан: " + snapshot.id +
                      " (добавлено: "  + std::to_string(added) +
                      ", изменено: "   + std::to_string(modified) +
                      ", удалено: "    + std::to_string(deleted) + ")"};
    } catch (const std::exception& e) {
        return {false, std::string("Ошибка: ") + e.what()};
    }
}

Result BackupManager::restore(const std::string& snapshotId, const std::string& backupDir) {
    try {
        auto& storage = StorageEngine::getInstance();
        std::string absBackupDir = fs::absolute(backupDir).string();

        notify("Загрузка снапшота " + snapshotId + "...");
        Snapshot target = storage.loadSnapshot(snapshotId, absBackupDir);
        std::string dst = target.srcPath;

        std::vector<Snapshot> chain;
        Snapshot current = target;
        while (true) {
            chain.insert(chain.begin(), current);
            if (current.type == SnapshotType::FULL) break;
            current = storage.loadSnapshot(current.parentId, absBackupDir);
        }

        notify("Восстановление в: " + dst + " (цепочка: " +
               std::to_string(chain.size()) + " снапшотов)");

        fs::create_directories(dst);

        for (const auto& snap : chain) {
            std::string snapshotPath = (fs::path(absBackupDir) / ".snapshots" / snap.id).string();

            std::vector<std::string> filesToRestore;
            for (const auto& change : snap.changes) {
                if (change.status == ChangeStatus::DELETED) {
                    fs::path toRemove = fs::path(dst) / change.path;
                    if (fs::exists(toRemove)) fs::remove(toRemove);
                } else {
                    filesToRestore.push_back(change.path);
                }
            }
            storage.restoreFiles(snapshotPath, filesToRestore, dst);
        }

        return {true, "Восстановление завершено из снапшота: " + snapshotId};
    } catch (const std::exception& e) {
        return {false, std::string("Ошибка: ") + e.what()};
    }
}

Result BackupManager::deleteSnapshot(const std::string& snapshotId, const std::string& backupDir) {
    try {
        StorageEngine::getInstance().deleteSnapshot(snapshotId, fs::absolute(backupDir).string());
        return {true, "Снапшот удалён: " + snapshotId};
    } catch (const std::exception& e) {
        return {false, std::string("Ошибка: ") + e.what()};
    }
}

std::vector<Snapshot> BackupManager::listSnapshots(const std::string& backupDir) {
    return StorageEngine::getInstance().listSnapshots(fs::absolute(backupDir).string());
}