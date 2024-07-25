#ifndef CROSSPROCESSLOCK_HPP
#define CROSSPROCESSLOCK_HPP

#include <windows.h>
#include <string>

enum class LockType {
    Read,
    Write
};

class CrossProcessLock {
    public:
        CrossProcessLock(std::wstring lockName);
        ~CrossProcessLock();
        
        DWORD lock(LockType lockType);
        DWORD release();

    private:
        HANDLE writeMutex;
        HANDLE readMutex;
        HANDLE sharedMemory;
        int* readCounter;
        LockType lockType;
};

#endif // CROSSPROCESSLOCK_HPP