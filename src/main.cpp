#include "cli/CLI.h"
#include "core/BackupManager.h"
#include "core/FileIndexer.h"
#include "core/CRC32Strategy.h"
#include <memory>
#ifdef _WIN32
#include <windows.h>
#endif
#include <clocale>

int main(int argc, char* argv[]) {
    #ifdef _WIN32
        SetConsoleOutputCP(CP_UTF8);
        SetConsoleCP(CP_UTF8);
    #else
        std::setlocale(LC_ALL, "");
    #endif
    
    auto hashStrategy = std::make_unique<CRC32Strategy>();
    auto indexer = std::make_unique<FileIndexer>(std::move(hashStrategy));
    BackupManager manager(std::move(indexer));

    CLI cli(manager);
    cli.run(argc, argv);

    return 0;
}