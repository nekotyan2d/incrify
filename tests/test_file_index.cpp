#include "catch2/catch_amalgamated.hpp"
#include "models/FileIndex.h"

TEST_CASE("FileIndex::set и getHash", "[FileIndex]") {

    SECTION("получение хэша существующего файла") {
        FileIndex index;
        index.set("file.txt", "abc123");
        REQUIRE(index.getHash("file.txt") == "abc123");
    }

    SECTION("получение хэша несуществующего файла возвращает пустую строку") {
        FileIndex index;
        REQUIRE(index.getHash("nonexistent.txt") == "");
    }

    SECTION("перезапись хэша существующего файла") {
        FileIndex index;
        index.set("file.txt", "abc123");
        index.set("file.txt", "xyz789");
        REQUIRE(index.getHash("file.txt") == "xyz789");
    }

    SECTION("несколько файлов хранятся независимо") {
        FileIndex index;
        index.set("a.txt", "hash_a");
        index.set("b.txt", "hash_b");
        index.set("c.txt", "hash_c");
        REQUIRE(index.getHash("a.txt") == "hash_a");
        REQUIRE(index.getHash("b.txt") == "hash_b");
        REQUIRE(index.getHash("c.txt") == "hash_c");
    }

    SECTION("вложенный путь как ключ") {
        FileIndex index;
        index.set("subdir/nested/file.txt", "hash123");
        REQUIRE(index.getHash("subdir/nested/file.txt") == "hash123");
    }

    SECTION("пустая строка как путь") {
        FileIndex index;
        index.set("", "emptyhash");
        REQUIRE(index.getHash("") == "emptyhash");
    }

    SECTION("хэш с пустым значением") {
        FileIndex index;
        index.set("file.txt", "");
        REQUIRE(index.getHash("file.txt") == "");
    }
}

TEST_CASE("FileIndex::hasFile", "[FileIndex]") {

    SECTION("возвращает true для существующего файла") {
        FileIndex index;
        index.set("file.txt", "hash");
        REQUIRE(index.hasFile("file.txt") == true);
    }

    SECTION("возвращает false для несуществующего файла") {
        FileIndex index;
        REQUIRE(index.hasFile("missing.txt") == false);
    }

    SECTION("возвращает false для пустого индекса") {
        FileIndex index;
        REQUIRE(index.hasFile("any.txt") == false);
    }

    SECTION("после перезаписи файл всё ещё существует") {
        FileIndex index;
        index.set("file.txt", "v1");
        index.set("file.txt", "v2");
        REQUIRE(index.hasFile("file.txt") == true);
    }

    SECTION("похожие пути не путаются между собой") {
        FileIndex index;
        index.set("file.txt", "hash");
        REQUIRE(index.hasFile("file.tx") == false);
        REQUIRE(index.hasFile("file.txt2") == false);
        REQUIRE(index.hasFile("File.txt") == false);
    }
}

TEST_CASE("FileIndex::entries", "[FileIndex]") {

    SECTION("пустой индекс возвращает пустую map") {
        FileIndex index;
        REQUIRE(index.entries().empty() == true);
    }

    SECTION("размер entries совпадает с количеством уникальных файлов") {
        FileIndex index;
        index.set("a.txt", "h1");
        index.set("b.txt", "h2");
        index.set("c.txt", "h3");
        REQUIRE(index.entries().size() == 3);
    }

    SECTION("перезапись не увеличивает размер") {
        FileIndex index;
        index.set("a.txt", "h1");
        index.set("a.txt", "h2");
        REQUIRE(index.entries().size() == 1);
    }

    SECTION("entries содержит все добавленные пары") {
        FileIndex index;
        index.set("x.txt", "hashX");
        index.set("y.txt", "hashY");
        const auto& e = index.entries();
        REQUIRE(e.at("x.txt") == "hashX");
        REQUIRE(e.at("y.txt") == "hashY");
    }
}