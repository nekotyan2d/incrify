#include "catch2/catch_amalgamated.hpp"
#include "core/BackupManager.h"
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

static BackupManager makeManager() {
    return BackupManager(
        std::make_unique<FileIndexer>(
            std::make_unique<CRC32Strategy>()));
}

struct TempDirs {
    fs::path src;
    fs::path backup;

    TempDirs() {
        src    = fs::temp_directory_path() / "incrify_test_bm_src";
        backup = fs::temp_directory_path() / "incrify_test_bm_backup";
        fs::create_directories(src);
        fs::create_directories(backup);
    }
    ~TempDirs() {
        fs::remove_all(src);
        fs::remove_all(backup);
    }
};

TEST_CASE("BackupManager::createFullBackup", "[BackupManager]") {

    SECTION("успешный полный бэкап возвращает success=true") {
        TempDirs tmp;
        auto manager = makeManager();
        writeFile(tmp.src / "file.txt", "hello");
        auto result = manager.createFullBackup(tmp.src.string(), tmp.backup.string());
        REQUIRE(result.success == true);
    }

    SECTION("после бэкапа появляется один снапшот") {
        TempDirs tmp;
        auto manager = makeManager();
        writeFile(tmp.src / "file.txt", "hello");
        manager.createFullBackup(tmp.src.string(), tmp.backup.string());
        auto list = manager.listSnapshots(tmp.backup.string());
        REQUIRE(list.size() == 1);
    }

    SECTION("созданный снапшот имеет тип FULL") {
        TempDirs tmp;
        auto manager = makeManager();
        writeFile(tmp.src / "file.txt", "hello");
        manager.createFullBackup(tmp.src.string(), tmp.backup.string());
        auto list = manager.listSnapshots(tmp.backup.string());
        REQUIRE(list[0].type == SnapshotType::FULL);
    }

    SECTION("снапшот FULL не имеет родителя") {
        TempDirs tmp;
        auto manager = makeManager();
        writeFile(tmp.src / "file.txt", "hello");
        manager.createFullBackup(tmp.src.string(), tmp.backup.string());
        auto list = manager.listSnapshots(tmp.backup.string());
        REQUIRE(list[0].parentId.empty());
    }

    SECTION("бэкап пустой директории возвращает success=true") {
        TempDirs tmp;
        auto manager = makeManager();
        auto result = manager.createFullBackup(tmp.src.string(), tmp.backup.string());
        REQUIRE(result.success == true);
    }

    SECTION("бэкап несуществующей директории возвращает success=false") {
        TempDirs tmp;
        auto manager = makeManager();
        auto result = manager.createFullBackup("/nonexistent/path", tmp.backup.string());
        REQUIRE(result.success == false);
    }

    SECTION("индекс снапшота содержит все файлы источника") {
        TempDirs tmp;
        auto manager = makeManager();
        writeFile(tmp.src / "a.txt", "aaa");
        writeFile(tmp.src / "b.txt", "bbb");
        manager.createFullBackup(tmp.src.string(), tmp.backup.string());
        auto list = manager.listSnapshots(tmp.backup.string());
        REQUIRE(list[0].index.entries().size() == 2);
    }
}

TEST_CASE("BackupManager::createIncremental", "[BackupManager]") {

    SECTION("инкремент без изменений возвращает success=true") {
        TempDirs tmp;
        auto manager = makeManager();
        writeFile(tmp.src / "file.txt", "hello");
        manager.createFullBackup(tmp.src.string(), tmp.backup.string());
        auto result = manager.createIncremental(tmp.src.string(), tmp.backup.string());
        REQUIRE(result.success == true);
    }

    SECTION("инкремент без изменений не создаёт новый снапшот") {
        TempDirs tmp;
        auto manager = makeManager();
        writeFile(tmp.src / "file.txt", "hello");
        manager.createFullBackup(tmp.src.string(), tmp.backup.string());
        manager.createIncremental(tmp.src.string(), tmp.backup.string());
        auto list = manager.listSnapshots(tmp.backup.string());
        REQUIRE(list.size() == 1);
    }

    SECTION("инкремент с изменениями создаёт новый снапшот") {
        TempDirs tmp;
        auto manager = makeManager();
        writeFile(tmp.src / "file.txt", "hello");
        manager.createFullBackup(tmp.src.string(), tmp.backup.string());
        writeFile(tmp.src / "file.txt", "modified");
        manager.createIncremental(tmp.src.string(), tmp.backup.string());
        auto list = manager.listSnapshots(tmp.backup.string());
        REQUIRE(list.size() == 2);
    }

    SECTION("созданный инкремент имеет тип INCREMENTAL") {
        TempDirs tmp;
        auto manager = makeManager();
        writeFile(tmp.src / "file.txt", "hello");
        manager.createFullBackup(tmp.src.string(), tmp.backup.string());
        writeFile(tmp.src / "new.txt", "new content");
        manager.createIncremental(tmp.src.string(), tmp.backup.string());
        auto list = manager.listSnapshots(tmp.backup.string());
        REQUIRE(list.size() == 2);
        REQUIRE(list[1].type == SnapshotType::INCREMENTAL);
    }

    SECTION("инкремент ссылается на предыдущий снапшот через parentId") {
        TempDirs tmp;
        auto manager = makeManager();
        writeFile(tmp.src / "file.txt", "hello");
        manager.createFullBackup(tmp.src.string(), tmp.backup.string());
        writeFile(tmp.src / "new.txt", "data");
        manager.createIncremental(tmp.src.string(), tmp.backup.string());
        auto list = manager.listSnapshots(tmp.backup.string());
        REQUIRE(list.size() == 2);
        REQUIRE(list[1].parentId == list[0].id);
    }

    SECTION("инкремент без предшествующего полного бэкапа возвращает success=false") {
        TempDirs tmp;
        auto manager = makeManager();
        writeFile(tmp.src / "file.txt", "hello");
        auto result = manager.createIncremental(tmp.src.string(), tmp.backup.string());
        REQUIRE(result.success == false);
    }

    SECTION("добавленный файл фиксируется в изменениях инкремента") {
        TempDirs tmp;
        auto manager = makeManager();
        writeFile(tmp.src / "file.txt", "hello");
        manager.createFullBackup(tmp.src.string(), tmp.backup.string());
        writeFile(tmp.src / "new.txt", "new");
        manager.createIncremental(tmp.src.string(), tmp.backup.string());
        auto list = manager.listSnapshots(tmp.backup.string());
        REQUIRE(list.size() == 2);
        bool found = false;
        for (const auto& c : list[1].changes)
            if (c.path.find("new.txt") != std::string::npos &&
                c.status == ChangeStatus::ADDED) found = true;
        REQUIRE(found);
    }
}

TEST_CASE("BackupManager::restore", "[BackupManager]") {

    SECTION("восстановление полного бэкапа возвращает success=true") {
        TempDirs tmp;
        auto manager = makeManager();
        writeFile(tmp.src / "file.txt", "original");
        manager.createFullBackup(tmp.src.string(), tmp.backup.string());
        auto list = manager.listSnapshots(tmp.backup.string());
        auto result = manager.restore(list[0].id, tmp.backup.string());
        REQUIRE(result.success == true);
    }

    SECTION("после восстановления файл имеет исходное содержимое") {
        TempDirs tmp;
        auto manager = makeManager();
        writeFile(tmp.src / "file.txt", "original content");
        manager.createFullBackup(tmp.src.string(), tmp.backup.string());

        // меняем файл
        writeFile(tmp.src / "file.txt", "changed content");
        auto list = manager.listSnapshots(tmp.backup.string());
        manager.restore(list[0].id, tmp.backup.string());

        std::ifstream f(tmp.src / "file.txt");
        std::string content((std::istreambuf_iterator<char>(f)),
                             std::istreambuf_iterator<char>());
        REQUIRE(content == "original content");
    }

    SECTION("восстановление инкремента проигрывает всю цепочку") {
        TempDirs tmp;
        auto manager = makeManager();
        writeFile(tmp.src / "file.txt", "v1");
        manager.createFullBackup(tmp.src.string(), tmp.backup.string());
        writeFile(tmp.src / "file.txt", "v2");
        writeFile(tmp.src / "new.txt", "added");
        manager.createIncremental(tmp.src.string(), tmp.backup.string());

        // удаляем src и восстанавливаем
        fs::remove_all(tmp.src);
        fs::create_directories(tmp.src);
        auto list = manager.listSnapshots(tmp.backup.string());
        REQUIRE(list.size() == 2);
        manager.restore(list[1].id, tmp.backup.string());

        REQUIRE(fs::exists(tmp.src / "file.txt"));
        REQUIRE(fs::exists(tmp.src / "new.txt"));
    }

    SECTION("строгое восстановление удаляет лишние файлы") {
        TempDirs tmp;
        auto manager = makeManager();
        writeFile(tmp.src / "file.txt", "hello");
        manager.createFullBackup(tmp.src.string(), tmp.backup.string());

        // добавляем лишний файл после бэкапа
        writeFile(tmp.src / "extra.txt", "extra");
        auto list = manager.listSnapshots(tmp.backup.string());
        manager.restore(list[0].id, tmp.backup.string());

        REQUIRE_FALSE(fs::exists(tmp.src / "extra.txt"));
    }

    SECTION("восстановление несуществующего снапшота возвращает success=false") {
        TempDirs tmp;
        auto manager = makeManager();
        auto result = manager.restore("nonexistent_id", tmp.backup.string());
        REQUIRE(result.success == false);
    }
}

TEST_CASE("BackupManager::deleteSnapshot", "[BackupManager]") {

    SECTION("удаление снапшота без дочерних возвращает success=true") {
        TempDirs tmp;
        auto manager = makeManager();
        writeFile(tmp.src / "file.txt", "data");
        manager.createFullBackup(tmp.src.string(), tmp.backup.string());
        auto list = manager.listSnapshots(tmp.backup.string());
        auto result = manager.deleteSnapshot(list[0].id, tmp.backup.string());
        REQUIRE(result.success == true);
    }

    SECTION("после удаления снапшот исчезает из списка") {
        TempDirs tmp;
        auto manager = makeManager();
        writeFile(tmp.src / "file.txt", "data");
        manager.createFullBackup(tmp.src.string(), tmp.backup.string());
        auto list = manager.listSnapshots(tmp.backup.string());
        manager.deleteSnapshot(list[0].id, tmp.backup.string());
        REQUIRE(manager.listSnapshots(tmp.backup.string()).empty());
    }

    SECTION("удаление родительского снапшота без -f возвращает success=false") {
        TempDirs tmp;
        auto manager = makeManager();
        writeFile(tmp.src / "file.txt", "data");
        manager.createFullBackup(tmp.src.string(), tmp.backup.string());
        writeFile(tmp.src / "file.txt", "modified");
        manager.createIncremental(tmp.src.string(), tmp.backup.string());
        auto list = manager.listSnapshots(tmp.backup.string());
        auto result = manager.deleteSnapshot(list[0].id, tmp.backup.string(), false);
        REQUIRE(result.success == false);
    }

    SECTION("каскадное удаление с force=true удаляет все дочерние снапшоты") {
        TempDirs tmp;
        auto manager = makeManager();
        writeFile(tmp.src / "file.txt", "v1");
        manager.createFullBackup(tmp.src.string(), tmp.backup.string());
        writeFile(tmp.src / "file.txt", "v2");
        manager.createIncremental(tmp.src.string(), tmp.backup.string());
        writeFile(tmp.src / "file.txt", "v3");
        manager.createIncremental(tmp.src.string(), tmp.backup.string());
        auto list = manager.listSnapshots(tmp.backup.string());
        manager.deleteSnapshot(list[0].id, tmp.backup.string(), true);
        REQUIRE(manager.listSnapshots(tmp.backup.string()).empty());
    }

    SECTION("удаление несуществующего снапшота возвращает success=false") {
        TempDirs tmp;
        auto manager = makeManager();
        auto result = manager.deleteSnapshot("nonexistent_id", tmp.backup.string());
        REQUIRE(result.success == false);
    }
}

TEST_CASE("BackupManager::listSnapshots", "[BackupManager]") {

    SECTION("пустая директория бэкапа возвращает пустой список") {
        TempDirs tmp;
        auto manager = makeManager();
        auto list = manager.listSnapshots(tmp.backup.string());
        REQUIRE(list.empty());
    }

    SECTION("список содержит все созданные снапшоты") {
        TempDirs tmp;
        auto manager = makeManager();
        writeFile(tmp.src / "file.txt", "v1");
        manager.createFullBackup(tmp.src.string(), tmp.backup.string());
        writeFile(tmp.src / "file.txt", "v2");
        manager.createIncremental(tmp.src.string(), tmp.backup.string());
        auto list = manager.listSnapshots(tmp.backup.string());
        REQUIRE(list.size() == 2);
    }
}