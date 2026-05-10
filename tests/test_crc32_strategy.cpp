#include "catch2/catch_amalgamated.hpp"
#include "core/CRC32Strategy.h"
#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;

static void writeFile(const fs::path& path, const std::string& content) {
    fs::create_directories(path.parent_path());
    std::ofstream f(path);
    f << content;
}

struct TempDir {
    fs::path path;

    TempDir() : path(fs::temp_directory_path() / "incrify_test_crc32") {
        fs::create_directories(path);
    }
    ~TempDir() {
        fs::remove_all(path);
    }
};

TEST_CASE("CRC32Strategy::hash — базовые свойства", "[CRC32Strategy]") {
    TempDir tmp;
    CRC32Strategy strategy;

    SECTION("хэш одинакового содержимого совпадает") {
        writeFile(tmp.path / "a.txt", "hello world");
        writeFile(tmp.path / "b.txt", "hello world");
        REQUIRE(strategy.hash((tmp.path / "a.txt").string()) ==
                strategy.hash((tmp.path / "b.txt").string()));
    }

    SECTION("хэш разного содержимого различается") {
        writeFile(tmp.path / "a.txt", "hello world");
        writeFile(tmp.path / "b.txt", "hello world!");
        REQUIRE(strategy.hash((tmp.path / "a.txt").string()) !=
                strategy.hash((tmp.path / "b.txt").string()));
    }

    SECTION("хэш одного файла стабилен при повторных вызовах") {
        writeFile(tmp.path / "file.txt", "stable content");
        std::string h1 = strategy.hash((tmp.path / "file.txt").string());
        std::string h2 = strategy.hash((tmp.path / "file.txt").string());
        REQUIRE(h1 == h2);
    }

    SECTION("хэш возвращает непустую строку") {
        writeFile(tmp.path / "file.txt", "some data");
        REQUIRE_FALSE(strategy.hash((tmp.path / "file.txt").string()).empty());
    }

    SECTION("хэш пустого файла не бросает исключение и не пуст") {
        writeFile(tmp.path / "empty.txt", "");
        std::string h;
        REQUIRE_NOTHROW(h = strategy.hash((tmp.path / "empty.txt").string()));
        REQUIRE_FALSE(h.empty());
    }

    SECTION("хэш пустого файла отличается от хэша непустого") {
        writeFile(tmp.path / "empty.txt", "");
        writeFile(tmp.path / "nonempty.txt", "x");
        REQUIRE(strategy.hash((tmp.path / "empty.txt").string()) !=
                strategy.hash((tmp.path / "nonempty.txt").string()));
    }

    SECTION("изменение одного байта меняет хэш") {
        writeFile(tmp.path / "v1.txt", "abcdef");
        writeFile(tmp.path / "v2.txt", "abcdeF");
        REQUIRE(strategy.hash((tmp.path / "v1.txt").string()) !=
                strategy.hash((tmp.path / "v2.txt").string()));
    }
}

TEST_CASE("CRC32Strategy::hash — формат результата", "[CRC32Strategy]") {
    TempDir tmp;
    CRC32Strategy strategy;
    writeFile(tmp.path / "file.txt", "test");

    SECTION("результат имеет длину 8 символов (hex CRC32)") {
        std::string h = strategy.hash((tmp.path / "file.txt").string());
        REQUIRE(h.size() == 8);
    }

    SECTION("результат содержит только hex-символы") {
        std::string h = strategy.hash((tmp.path / "file.txt").string());
        REQUIRE(h.find_first_not_of("0123456789abcdef") == std::string::npos);
    }
}

TEST_CASE("CRC32Strategy::hash — ошибочные ситуации", "[CRC32Strategy]") {
    CRC32Strategy strategy;

    SECTION("несуществующий файл бросает исключение") {
        REQUIRE_THROWS(strategy.hash("/nonexistent/path/file.txt"));
    }
}