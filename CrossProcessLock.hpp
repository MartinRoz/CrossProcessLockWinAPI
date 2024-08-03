#pragma once
#include <windows.h>
#include <string>
#include <cstdint>
#include "ScopedHandle.hpp"

enum class LockType : uint8_t
{
    Unlocked,
    Read,
    Write
};


class CrossProcessLock
{
public:
    CrossProcessLock(const std::wstring& lockName);
    ~CrossProcessLock();
    
    DWORD lock(LockType lockType);
    DWORD release();

    LockType getLockType();

private:
    ScopedHandle writeMutex;
    ScopedHandle readMutex;
    ScopedHandle sharedMemory;
    int* readCounter;
    LockType lockType;
};