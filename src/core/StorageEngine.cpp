#include "core/StorageEngine.h"

namespace fs = std::filesystem;
using json = nlohmann::json;
 
StorageEngine& StorageEngine::getInstance() {
    static StorageEngine instance;
    return instance;
}
 
std::string StorageEngine::getSnapshotsDir(const std::string& dstPath) const {
    return (fs::path(dstPath) / ".snapshots").string();
}
 
std::string StorageEngine::getSnapshotPath(const std::string& dstPath, const std::string& id) const {
    return (fs::path(dstPath) / ".snapshots" / id).string();
}
 
std::string StorageEngine::getMetaPath(const std::string& dstPath, const std::string& id) const {
    return (fs::path(dstPath) / ".snapshots" / id / "meta.json").string();
}

void StorageEngine::saveSnapshot(const Snapshot& snapshot) {
    std::string snapshotDir = getSnapshotPath(snapshot.dstPath, snapshot.id);
    fs::create_directories(snapshotDir);
 
    json meta;
    meta["id"] = snapshot.id;
    meta["type"] = (snapshot.type == SnapshotType::FULL) ? "FULL" : "INCREMENTAL";
    meta["parentId"] = snapshot.parentId;
    meta["timestamp"] = snapshot.timestamp;
    meta["srcPath"] = snapshot.srcPath;
    meta["dstPath"] = snapshot.dstPath;
 
    json indexJson;
    for (const auto& [path, hash] : snapshot.index.entries())
        indexJson[path] = hash;
    meta["index"] = indexJson;
 
    json changesJson = json::array();
    for (const auto& change : snapshot.changes) {
        std::string status;
        switch (change.status) {
            case ChangeStatus::ADDED:    status = "ADDED";    break;
            case ChangeStatus::MODIFIED: status = "MODIFIED"; break;
            case ChangeStatus::DELETED:  status = "DELETED";  break;
        }
        changesJson.push_back({
            {"path", change.path},
            {"status", status},
            {"hash", change.hash}
        });
    }
    meta["changes"] = changesJson;
 
    std::ofstream metaFile(getMetaPath(snapshot.dstPath, snapshot.id));
    metaFile << meta.dump(2);
}
 
Snapshot StorageEngine::loadSnapshot(const std::string& id, const std::string& dstPath) const {
    std::string metaPath = getMetaPath(dstPath, id);
    std::ifstream metaFile(metaPath);
    if (!metaFile.is_open())
        throw std::runtime_error("Снапшот не найден: " + id);
 
    json meta;
    metaFile >> meta;
 
    Snapshot snapshot;
    snapshot.id = meta["id"];
    snapshot.type = (meta["type"] == "FULL") ? SnapshotType::FULL : SnapshotType::INCREMENTAL;
    snapshot.parentId = meta["parentId"];
    snapshot.timestamp = meta["timestamp"];
    snapshot.srcPath = meta["srcPath"];
    snapshot.dstPath = meta["dstPath"];
 
    for (const auto& [path, hash] : meta["index"].items())
        snapshot.index.set(path, hash.get<std::string>());
 
    for (const auto& c : meta["changes"]) {
        FileChange change;
        change.path = c["path"];
        change.hash = c["hash"];
        std::string status = c["status"];
        if(status == "ADDED") change.status = ChangeStatus::ADDED;
        else if (status == "MODIFIED") change.status = ChangeStatus::MODIFIED;
        else change.status = ChangeStatus::DELETED;
        snapshot.changes.push_back(change);
    }
 
    return snapshot;
}

std::vector<Snapshot> StorageEngine::listSnapshots(const std::string& dstPath) const {
    std::vector<Snapshot> snapshots;
    std::string snapshotsDir = getSnapshotsDir(dstPath);
 
    if (!fs::exists(snapshotsDir)) return snapshots;
 
    for (const auto& entry : fs::directory_iterator(snapshotsDir)) {
        if (!entry.is_directory()) continue;
        try {
            snapshots.push_back(loadSnapshot(entry.path().filename().string(), dstPath));
        } catch (...) { /* пропускаем повреждённые */ }
    }
 
    std::sort(snapshots.begin(), snapshots.end(),
              [](const Snapshot& a, const Snapshot& b) {
                  return a.timestamp < b.timestamp;
              });
 
    return snapshots;
}
 
Snapshot StorageEngine::loadLastSnapshot(const std::string& dstPath) const {
    auto snapshots = listSnapshots(dstPath);
    if (snapshots.empty())
        throw std::runtime_error("Снапшотов не найдено в: " + dstPath);
    return snapshots.back();
}
 
void StorageEngine::deleteSnapshot(const std::string& id, const std::string& dstPath) {
    std::string path = getSnapshotPath(dstPath, id);
    if (!fs::exists(path))
        throw std::runtime_error("Снапшот не найден: " + id);
    fs::remove_all(path);
}
 
void StorageEngine::copyFiles(const std::string& srcPath, const std::vector<FileChange>& changes, const std::string& snapshotPath) const {
    fs::path filesDir = fs::path(snapshotPath) / "files";
 
    for (const auto& change : changes) {
        if (change.status == ChangeStatus::DELETED) continue;
 
        fs::path src = fs::path(srcPath) / change.path;
        fs::path dst = filesDir / change.path;
 
        fs::create_directories(dst.parent_path());
        if(fs::exists(dst)){
            fs::remove(dst);
        }
        fs::copy_file(src, dst);
    }
}
 
void StorageEngine::restoreFiles(const std::string& snapshotPath, const std::vector<std::string>& filePaths, const std::string& dstPath) const {
    fs::path filesDir = fs::path(snapshotPath) / "files";
 
    for (const auto& filePath : filePaths) {
        fs::path src = filesDir / filePath;
        fs::path dst = fs::path(dstPath) / filePath;
 
        if (!fs::exists(src)) continue;
 
        fs::create_directories(dst.parent_path());
        if(fs::exists(dst)){
            fs::remove(dst);
        }
        fs::copy_file(src, dst);
    }
}
 