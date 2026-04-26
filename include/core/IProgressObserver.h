#pragma once
#include <string>

class IProgressObserver {
    public:
        virtual void onProgress(const std::string& message) = 0;
        virtual ~IProgressObserver() = default;
};