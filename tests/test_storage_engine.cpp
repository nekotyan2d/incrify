#include "catch2/catch_amalgamated.hpp"
#include "core/StorageEngine.h"
#include "models/Snapshot.h"
#include <filesystem>

namespace fs = std::filesystem;

struct TempDir {
    fs::path path;

    TempDir() : path(fs::temp_directory_path() / "incrify_test_storage") {
        fs::create_directories(path);
    }
    ~TempDir() {
        fs::remove_all(path);
    }
};

static Snapshot makeSnapshot(const std::string& id,
                              SnapshotType type,
                              const std::string& parentId,
                              const std::string& dstPath) {
    Snapshot s;
    s.id = id;
    s.type = type;
    s.parentId = parentId;
    s.timestamp = id;
    s.srcPath = "/test/src";
    s.dstPath = dstPath;
    s.index.set("file.txt", "abc123");
    s.changes.push_back({"file.txt", ChangeStatus::ADDED, "abc123"});
    return s;
}

TEST_CASE("StorageEngine::getInstance", "[StorageEngine]") {

    SECTION("два вызова возвращают один и тот же объект") {
        auto& a = StorageEngine::getInstance();
        auto& b = StorageEngine::getInstance();
        REQUIRE(&a == &b);
    }
}

TEST_CASE("StorageEngine::saveSnapshot и loadSnapshot", "[StorageEngine]") {
    TempDir tmp;
    auto& storage = StorageEngine::getInstance();
    std::string dst = tmp.path.string();

    SECTION("сохранённый снапшот можно загрузить") {
        auto snap = makeSnapshot("20240101_120000_000", SnapshotType::FULL, "", dst);
        storage.saveSnapshot(snap);
        auto loaded = storage.loadSnapshot(snap.id, dst);
        REQUIRE(loaded.id == snap.id);
    }

    SECTION("тип снапшота сохраняется корректно — FULL") {
        auto snap = makeSnapshot("20240101_120000_001", SnapshotType::FULL, "", dst);
        storage.saveSnapshot(snap);
        auto loaded = storage.loadSnapshot(snap.id, dst);
        REQUIRE(loaded.type == SnapshotType::FULL);
    }

    SECTION("тип снапшота сохраняется корректно — INCREMENTAL") {
        auto snap = makeSnapshot("20240101_120000_002", SnapshotType::INCREMENTAL,
                                 "20240101_120000_001", dst);
        storage.saveSnapshot(snap);
        auto loaded = storage.loadSnapshot(snap.id, dst);
        REQUIRE(loaded.type == SnapshotType::INCREMENTAL);
    }

    SECTION("parentId сохраняется и загружается") {
        auto snap = makeSnapshot("20240101_120000_003", SnapshotType::INCREMENTAL,
                                 "parent_id_xyz", dst);
        storage.saveSnapshot(snap);
        auto loaded = storage.loadSnapshot(snap.id, dst);
        REQUIRE(loaded.parentId == "parent_id_xyz");
    }

    SECTION("srcPath сохраняется и загружается") {
        auto snap = makeSnapshot("20240101_120000_004", SnapshotType::FULL, "", dst);
        snap.srcPath = "/custom/source/path";
        storage.saveSnapshot(snap);
        auto loaded = storage.loadSnapshot(snap.id, dst);
        REQUIRE(loaded.srcPath == "/custom/source/path");
    }

    SECTION("индекс файлов сохраняется и загружается") {
        auto snap = makeSnapshot("20240101_120000_005", SnapshotType::FULL, "", dst);
        snap.index.set("extra.txt", "hash999");
        storage.saveSnapshot(snap);
        auto loaded = storage.loadSnapshot(snap.id, dst);
        REQUIRE(loaded.index.hasFile("file.txt"));
        REQUIRE(loaded.index.getHash("file.txt") == "abc123");
        REQUIRE(loaded.index.hasFile("extra.txt"));
        REQUIRE(loaded.index.getHash("extra.txt") == "hash999");
    }

    SECTION("список изменений сохраняется и загружается") {
        auto snap = makeSnapshot("20240101_120000_006", SnapshotType::FULL, "", dst);
        snap.changes.push_back({"deleted.txt", ChangeStatus::DELETED, ""});
        storage.saveSnapshot(snap);
        auto loaded = storage.loadSnapshot(snap.id, dst);
        REQUIRE(loaded.changes.size() == snap.changes.size());
    }

    SECTION("загрузка несуществующего снапшота бросает исключение") {
        REQUIRE_THROWS(storage.loadSnapshot("nonexistent_id", dst));
    }
}

TEST_CASE("StorageEngine::listSnapshots", "[StorageEngine]") {
    TempDir tmp;
    auto& storage = StorageEngine::getInstance();
    std::string dst = tmp.path.string();

    SECTION("пустая директория возвращает пустой список") {
        auto list = storage.listSnapshots(dst);
        REQUIRE(list.empty());
    }

    SECTION("один снапшот — список из одного элемента") {
        storage.saveSnapshot(
            makeSnapshot("20240101_120000_010", SnapshotType::FULL, "", dst));
        auto list = storage.listSnapshots(dst);
        REQUIRE(list.size() == 1);
    }

    SECTION("несколько снапшотов — все присутствуют в списке") {
        storage.saveSnapshot(
            makeSnapshot("20240101_120000_020", SnapshotType::FULL, "", dst));
        storage.saveSnapshot(
            makeSnapshot("20240101_120000_021", SnapshotType::INCREMENTAL,
                         "20240101_120000_020", dst));
        storage.saveSnapshot(
            makeSnapshot("20240101_120000_022", SnapshotType::INCREMENTAL,
                         "20240101_120000_021", dst));
        auto list = storage.listSnapshots(dst);
        REQUIRE(list.size() == 3);
    }

    SECTION("снапшоты отсортированы по timestamp по возрастанию") {
        storage.saveSnapshot(
            makeSnapshot("20240101_120000_030", SnapshotType::FULL, "", dst));
        storage.saveSnapshot(
            makeSnapshot("20240101_120000_032", SnapshotType::INCREMENTAL,
                         "20240101_120000_030", dst));
        storage.saveSnapshot(
            makeSnapshot("20240101_120000_031", SnapshotType::INCREMENTAL,
                         "20240101_120000_030", dst));
        auto list = storage.listSnapshots(dst);
        REQUIRE(list[0].id == "20240101_120000_030");
        REQUIRE(list[1].id == "20240101_120000_031");
        REQUIRE(list[2].id == "20240101_120000_032");
    }
}

TEST_CASE("StorageEngine::loadLastSnapshot", "[StorageEngine]") {
    TempDir tmp;
    auto& storage = StorageEngine::getInstance();
    std::string dst = tmp.path.string();

    SECTION("возвращает последний по timestamp снапшот") {
        storage.saveSnapshot(
            makeSnapshot("20240101_120000_040", SnapshotType::FULL, "", dst));
        storage.saveSnapshot(
            makeSnapshot("20240101_120000_041", SnapshotType::INCREMENTAL,
                         "20240101_120000_040", dst));
        auto last = storage.loadLastSnapshot(dst);
        REQUIRE(last.id == "20240101_120000_041");
    }

    SECTION("пустая директория бросает исключение") {
        REQUIRE_THROWS(storage.loadLastSnapshot(dst));
    }
}

TEST_CASE("StorageEngine::deleteSnapshot", "[StorageEngine]") {
    TempDir tmp;
    auto& storage = StorageEngine::getInstance();
    std::string dst = tmp.path.string();

    SECTION("удалённый снапшот исчезает из списка") {
        storage.saveSnapshot(
            makeSnapshot("20240101_120000_050", SnapshotType::FULL, "", dst));
        storage.deleteSnapshot("20240101_120000_050", dst);
        auto list = storage.listSnapshots(dst);
        REQUIRE(list.empty());
    }

    SECTION("удаление несуществующего снапшота бросает исключение") {
        REQUIRE_THROWS(storage.deleteSnapshot("nonexistent_id", dst));
    }

    SECTION("удаление одного снапшота не затрагивает остальные") {
        storage.saveSnapshot(
            makeSnapshot("20240101_120000_060", SnapshotType::FULL, "", dst));
        storage.saveSnapshot(
            makeSnapshot("20240101_120000_061", SnapshotType::INCREMENTAL,
                         "20240101_120000_060", dst));
        storage.deleteSnapshot("20240101_120000_061", dst);
        auto list = storage.listSnapshots(dst);
        REQUIRE(list.size() == 1);
        REQUIRE(list[0].id == "20240101_120000_060");
    }
}