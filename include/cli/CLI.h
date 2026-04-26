#pragma once
#include <vector>
#include <string>
#include <memory>
#include "core/BackupManager.h"
#include "ICommand.h"

class CLI {
    public:
        explicit CLI(BackupManager& manager);
        void run(int argc, char* argv[]);

    private:
        BackupManager& _manager;
        
        std::unique_ptr<ICommand> parseCommand(const std::vector<std::string>& args) const;
        void printUsage() const;
};