#include "cli/CLI.h"

class ConsoleProgressObserver : public IProgressObserver {
public:
    void onProgress(const std::string& message) override {
        std::cout << "  >> " << message << "\n";
    }
};

class BackupFullCommand : public ICommand {
    BackupManager& _manager;
    std::string _src, _dst;
public:
    BackupFullCommand(BackupManager& m, std::string src, std::string dst)
        : _manager(m), _src(std::move(src)), _dst(std::move(dst)) {}

    void execute() override {
        std::cout << "[backup full] " << _src << " -> " << _dst << "\n";
        auto result = _manager.createFullBackup(_src, _dst);
        std::cout << (result.success ? "[OK] " : "[ОШИБКА] ") << result.message << "\n";
    }
};

class BackupIncrementalCommand : public ICommand {
    BackupManager& _manager;
    std::string _src, _dst;
public:
    BackupIncrementalCommand(BackupManager& m, std::string src, std::string dst)
        : _manager(m), _src(std::move(src)), _dst(std::move(dst)) {}

    void execute() override {
        std::cout << "[backup incremental] " << _src << " -> " << _dst << "\n";
        auto result = _manager.createIncremental(_src, _dst);
        std::cout << (result.success ? "[OK] " : "[ОШИБКА] ") << result.message << "\n";
    }
};

class ListCommand : public ICommand {
    BackupManager& _manager;
    std::string _backupDir;
public:
    ListCommand(BackupManager& m, std::string dir)
        : _manager(m), _backupDir(std::move(dir)) {}

    void execute() override {
        auto snapshots = _manager.listSnapshots(_backupDir);
        if (snapshots.empty()) {
            std::cout << "Снапшотов не найдено в: " << _backupDir << "\n";
            return;
        }
        std::cout << "Снапшоты в: " << _backupDir << "\n";
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
    BackupManager& _manager;
    std::string _snapshotId, _backupDir;
public:
    RestoreCommand(BackupManager& m, std::string id, std::string dir)
        : _manager(m), _snapshotId(std::move(id)), _backupDir(std::move(dir)) {}

    void execute() override {
        std::cout << "[restore] " << _snapshotId << "\n";
        auto result = _manager.restore(_snapshotId, _backupDir);
        std::cout << (result.success ? "[OK] " : "[ОШИБКА] ") << result.message << "\n";
    }
};

class DeleteCommand : public ICommand {
    BackupManager& _manager;
    std::string _snapshotId, _backupDir;
public:
    DeleteCommand(BackupManager& m, std::string id, std::string dir)
        : _manager(m), _snapshotId(std::move(id)), _backupDir(std::move(dir)) {}

    void execute() override {
        auto result = _manager.deleteSnapshot(_snapshotId, _backupDir);
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