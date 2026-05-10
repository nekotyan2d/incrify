#include "catch2/catch_amalgamated.hpp"
#include "core/FileIndexer.h"

#include "core/CRC32Strategy.h"
#include <filesystem>
#include <fstream>
#include <memory>

namespace fs = std::filesystem;

static void writeFile(const fs::path& path, const std::string& content) {
    fs::create_directories(path.parent_path());
    std::ofstream f(path);
    f << content;
}

static FileIndexer makeIndexer() {
    return FileIndexer(std::make_unique<CRC32Strategy>());
}

struct TempDir {
    fs::path path;

    TempDir() : path(fs::temp_directory_path() / "incrify_test_indexer") {
        fs::create_directories(path);
    }
    ~TempDir() {
        fs::remove_all(path);
    }
};

TEST_CASE("FileIndexer::buildIndex", "[FileIndexer]") {
    TempDir tmp;
    auto indexer = makeIndexer();

    SECTION("пустая директория даёт пустой индекс") {
        auto index = indexer.buildIndex(tmp.path.string());
        REQUIRE(index.entries().empty());
    }

    SECTION("один файл попадает в индекс") {
        writeFile(tmp.path / "file.txt", "hello");
        auto index = indexer.buildIndex(tmp.path.string());
        REQUIRE(index.hasFile("file.txt"));
    }

    SECTION("несколько файлов — все попадают в индекс") {
        writeFile(tmp.path / "a.txt", "aaa");
        writeFile(tmp.path / "b.txt", "bbb");
        writeFile(tmp.path / "c.txt", "ccc");
        auto index = indexer.buildIndex(tmp.path.string());
        REQUIRE(index.entries().size() == 3);
        REQUIRE(index.hasFile("a.txt"));
        REQUIRE(index.hasFile("b.txt"));
        REQUIRE(index.hasFile("c.txt"));
    }

    SECTION("вложенные файлы попадают в индекс с относительным путём") {
        writeFile(tmp.path / "sub" / "nested.txt", "data");
        auto index = indexer.buildIndex(tmp.path.string());
        // на Windows разделитель может быть \, проверяем наличие файла
        bool found = false;
        for (const auto& [path, hash] : index.entries())
            if (path.find("nested.txt") != std::string::npos) found = true;
        REQUIRE(found);
    }

    SECTION("хэши в индексе непустые") {
        writeFile(tmp.path / "file.txt", "content");
        auto index = indexer.buildIndex(tmp.path.string());
        REQUIRE_FALSE(index.getHash("file.txt").empty());
    }

    SECTION("директории не попадают в индекс — только файлы") {
        writeFile(tmp.path / "sub" / "file.txt", "x");
        auto index = indexer.buildIndex(tmp.path.string());
        // в индексе должен быть файл, но не директория "sub"
        bool hasDir = false;
        for (const auto& [path, hash] : index.entries())
            if (path == "sub" || path == "sub/") hasDir = true;
        REQUIRE_FALSE(hasDir);
    }

    SECTION("несуществующая директория бросает исключение") {
        REQUIRE_THROWS(indexer.buildIndex("/nonexistent/path/dir"));
    }
}

TEST_CASE("FileIndexer::compare — добавленные файлы", "[FileIndexer]") {
    auto indexer = makeIndexer();

    SECTION("новый файл определяется как ADDED") {
        FileIndex oldIndex, newIndex;
        newIndex.set("new.txt", "hash1");
        auto changes = indexer.compare(oldIndex, newIndex);
        REQUIRE(changes.size() == 1);
        REQUIRE(changes[0].path == "new.txt");
        REQUIRE(changes[0].status == ChangeStatus::ADDED);
    }

    SECTION("несколько новых файлов — все ADDED") {
        FileIndex oldIndex, newIndex;
        newIndex.set("a.txt", "h1");
        newIndex.set("b.txt", "h2");
        newIndex.set("c.txt", "h3");
        auto changes = indexer.compare(oldIndex, newIndex);
        REQUIRE(changes.size() == 3);
        for (const auto& c : changes)
            REQUIRE(c.status == ChangeStatus::ADDED);
    }
}

TEST_CASE("FileIndexer::compare — удалённые файлы", "[FileIndexer]") {
    auto indexer = makeIndexer();

    SECTION("отсутствующий в новом индексе файл — DELETED") {
        FileIndex oldIndex, newIndex;
        oldIndex.set("old.txt", "hash1");
        auto changes = indexer.compare(oldIndex, newIndex);
        REQUIRE(changes.size() == 1);
        REQUIRE(changes[0].path == "old.txt");
        REQUIRE(changes[0].status == ChangeStatus::DELETED);
    }

    SECTION("хэш удалённого файла пуст") {
        FileIndex oldIndex, newIndex;
        oldIndex.set("old.txt", "hash1");
        auto changes = indexer.compare(oldIndex, newIndex);
        REQUIRE(changes[0].hash.empty());
    }
}

TEST_CASE("FileIndexer::compare — изменённые файлы", "[FileIndexer]") {
    auto indexer = makeIndexer();

    SECTION("файл с другим хэшем — MODIFIED") {
        FileIndex oldIndex, newIndex;
        oldIndex.set("file.txt", "oldhash");
        newIndex.set("file.txt", "newhash");
        auto changes = indexer.compare(oldIndex, newIndex);
        REQUIRE(changes.size() == 1);
        REQUIRE(changes[0].status == ChangeStatus::MODIFIED);
        REQUIRE(changes[0].hash == "newhash");
    }

    SECTION("файл с одинаковым хэшем не попадает в изменения") {
        FileIndex oldIndex, newIndex;
        oldIndex.set("file.txt", "samehash");
        newIndex.set("file.txt", "samehash");
        auto changes = indexer.compare(oldIndex, newIndex);
        REQUIRE(changes.empty());
    }
}

TEST_CASE("FileIndexer::compare — смешанные изменения", "[FileIndexer]") {
    auto indexer = makeIndexer();

    SECTION("одновременно ADDED, MODIFIED, DELETED") {
        FileIndex oldIndex, newIndex;
        oldIndex.set("unchanged.txt", "hash1");
        oldIndex.set("modified.txt", "oldHash");
        oldIndex.set("deleted.txt", "hash3");
        newIndex.set("unchanged.txt", "hash1");
        newIndex.set("modified.txt", "newHash");
        newIndex.set("added.txt", "hash4");

        auto changes = indexer.compare(oldIndex, newIndex);
        REQUIRE(changes.size() == 3);

        int added = 0, modified = 0, deleted = 0;
        for (const auto& c : changes) {
            if (c.status == ChangeStatus::ADDED)    added++;
            if (c.status == ChangeStatus::MODIFIED) modified++;
            if (c.status == ChangeStatus::DELETED)  deleted++;
        }
        REQUIRE(added    == 1);
        REQUIRE(modified == 1);
        REQUIRE(deleted  == 1);
    }

    SECTION("два одинаковых индекса — нет изменений") {
        FileIndex oldIndex, newIndex;
        oldIndex.set("a.txt", "h1");
        oldIndex.set("b.txt", "h2");
        newIndex.set("a.txt", "h1");
        newIndex.set("b.txt", "h2");
        auto changes = indexer.compare(oldIndex, newIndex);
        REQUIRE(changes.empty());
    }

    SECTION("оба индекса пустые — нет изменений") {
        FileIndex oldIndex, newIndex;
        auto changes = indexer.compare(oldIndex, newIndex);
        REQUIRE(changes.empty());
    }
}