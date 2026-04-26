#include "cli/CLI.h"
#include "cli/ICommand.h"
#include "core/IProgressObserver.h"
#include <iostream>
#include <iomanip>

class ConsoleProgressObserver : public IProgressObserver {
public:
    void onProgress(const std::string& message) override {
        std::cout << "  >> " << message << "\n";
    }
};

class BackupFullCommand : public ICommand {
    BackupManager& manager_;
    std::string src_, dst_;
public:
    BackupFullCommand(BackupManager& m, std::string src, std::string dst)
        : manager_(m), src_(std::move(src)), dst_(std::move(dst)) {}

    void execute() override {
        std::cout << "[backup full] " << src_ << " -> " << dst_ << "\n";
        auto result = manager_.createFullBackup(src_, dst_);
        std::cout << (result.success ? "[OK] " : "[ОШИБКА] ") << result.message << "\n";
    }
};

class BackupIncrementalCommand : public ICommand {
    BackupManager& manager_;
    std::string src_, dst_;
public:
    BackupIncrementalCommand(BackupManager& m, std::string src, std::string dst)
        : manager_(m), src_(std::move(src)), dst_(std::move(dst)) {}

    void execute() override {
        std::cout << "[backup incremental] " << src_ << " -> " << dst_ << "\n";
        auto result = manager_.createIncremental(src_, dst_);
        std::cout << (result.success ? "[OK] " : "[ОШИБКА] ") << result.message << "\n";
    }
};

class ListCommand : public ICommand {
    BackupManager& manager_;
    std::string backupDir_;
public:
    ListCommand(BackupManager& m, std::string dir)
        : manager_(m), backupDir_(std::move(dir)) {}

    void execute() override {
        auto snapshots = manager_.listSnapshots(backupDir_);
        if (snapshots.empty()) {
            std::cout << "Снапшотов не найдено в: " << backupDir_ << "\n";
            return;
        }
        std::cout << "Снапшоты в: " << backupDir_ << "\n";
        std::cout << std::string(62, '-') << "\n";
        std::cout << std::left
                  << std::setw(20) << "ID"
                  << std::setw(12) << "ТИП"
                  << "РОДИТЕЛЬ\n";
        std::cout << std::string(62, '-') << "\n";
        for (const auto& s : snapshots) {
            std::string type = (s.type == SnapshotType::FULL) ? "FULL" : "INCREMENTAL";
            std::cout << std::setw(20) << s.id
                      << std::setw(12) << type
                      << (s.parentId.empty() ? "-" : s.parentId) << "\n";
        }
        std::cout << std::string(62, '-') << "\n";
        std::cout << "Всего: " << snapshots.size() << "\n";
    }
};

class RestoreCommand : public ICommand {
    BackupManager& manager_;
    std::string snapshotId_, backupDir_;
public:
    RestoreCommand(BackupManager& m, std::string id, std::string dir)
        : manager_(m), snapshotId_(std::move(id)), backupDir_(std::move(dir)) {}

    void execute() override {
        std::cout << "[restore] " << snapshotId_ << "\n";
        auto result = manager_.restore(snapshotId_, backupDir_);
        std::cout << (result.success ? "[OK] " : "[ОШИБКА] ") << result.message << "\n";
    }
};

class DeleteCommand : public ICommand {
    BackupManager& manager_;
    std::string snapshotId_, backupDir_;
public:
    DeleteCommand(BackupManager& m, std::string id, std::string dir)
        : manager_(m), snapshotId_(std::move(id)), backupDir_(std::move(dir)) {}

    void execute() override {
        auto result = manager_.deleteSnapshot(snapshotId_, backupDir_);
        std::cout << (result.success ? "[OK] " : "[ОШИБКА] ") << result.message << "\n";
    }
};

static ConsoleProgressObserver consoleObserver;

CLI::CLI(BackupManager& manager) : _manager(manager) {
    _manager.addObserver(&consoleObserver);
}

void CLI::run(int argc, char* argv[]) {
    if (argc < 2) {
        printUsage();
        return;
    }

    std::vector<std::string> args(argv + 1, argv + argc);
    auto cmd = parseCommand(args);
    if (cmd) cmd->execute();
}

std::unique_ptr<ICommand> CLI::parseCommand(const std::vector<std::string>& args) const {
    const std::string& command = args[0];

    if (command == "full" && args.size() >= 3)
        return std::make_unique<BackupFullCommand>(_manager, args[1], args[2]);

    if (command == "incremental" && args.size() >= 3)
        return std::make_unique<BackupIncrementalCommand>(_manager, args[1], args[2]);

    if (command == "list" && args.size() >= 2)
        return std::make_unique<ListCommand>(_manager, args[1]);

    if (command == "restore" && args.size() >= 3)
        return std::make_unique<RestoreCommand>(_manager, args[1], args[2]);

    if (command == "delete" && args.size() >= 3)
        return std::make_unique<DeleteCommand>(_manager, args[1], args[2]);

    printUsage();
    return nullptr;
}

void CLI::printUsage() const {
    std::cout << "\nИспользование:\n"
              << "  incrify full <src> <dst>                - Полный бэкап\n"
              << "  incrify incremental <src> <dst>         - Инкрементальный бэкап\n"
              << "  incrify list <backup_dir>               - Список снапшотов\n"
              << "  incrify restore <snapshot_id> <dir>     - Восстановление\n"
              << "  incrify delete <snapshot_id> <dir>      - Удалить снапшот\n\n"
              << "Пример:\n"
              << "  incrify full ./my_docs ./my_backup\n"
              << "  incrify incremental ./my_docs ./my_backup\n"
              << "  incrify list ./my_backup\n"
              << "  incrify restore 20260428_143022 ./my_backup\n";
}