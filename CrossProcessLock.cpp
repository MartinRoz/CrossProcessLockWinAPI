#include "CrossProcessLock.hpp"
#include <stdexcept>

#define FIVE_SECONDS 5000

CrossProcessLock::CrossProcessLock(std::wstring lockName)
{
    DWORD mutexStatus;
    std::wstring writeMutexName = lockName + L"_write";
    std::wstring readMutexName = lockName + L"_read";
    std::wstring sharedMemoryName = lockName + L"_data";

    /* get or create write mutex */
    writeMutex = CreateMutexW(NULL, FALSE, writeMutexName.c_str());
    if (writeMutex == NULL)
        throw std::runtime_error("Failed to create mutex.");

    /* get or create read mutex */
    readMutex = CreateMutexW(NULL, TRUE, readMutexName.c_str());
    if (readMutex == NULL)
    {
        CloseHandle(writeMutex);
        throw std::runtime_error("Failed to create mutex.");
    }
    mutexStatus = GetLastError();

    /* get or create shared data */
    sharedMemory = CreateFileMappingW(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, sizeof(int), sharedMemoryName.c_str());
    if (sharedMemory == NULL)
    {
        CloseHandle(writeMutex);
        CloseHandle(readMutex);
        throw std::runtime_error("Failed to create shared memory.");
    }

    readCounter = (int*)MapViewOfFile(sharedMemory, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(int));
    if (readCounter == NULL)
    {
        CloseHandle(sharedMemory);
        CloseHandle(writeMutex);
        CloseHandle(readMutex);
        throw std::runtime_error("Failed to map view of shared memory.");
    }

    /* if the mutex was first time created, init data */
    if (mutexStatus != ERROR_ALREADY_EXISTS)
        *readCounter = 0;
    ReleaseMutex(readMutex);
}

CrossProcessLock::~CrossProcessLock()
{
    CloseHandle(writeMutex);     
    CloseHandle(readMutex);
    CloseHandle(sharedMemory);
    UnmapViewOfFile(readCounter);     
}

DWORD CrossProcessLock::lock(LockType lockType) 
{
    DWORD result;
    lockType = lockType;

    if (lockType == LockType::Unlocked)
        return WAIT_FAILED;

    if (lockType == LockType::Write)
        return WaitForSingleObject(writeMutex, FIVE_SECONDS);
    
    //on read lock
    result = WaitForSingleObject(readMutex, FIVE_SECONDS);
    if (result == WAIT_FAILED)
        return result;
    
    (*readCounter)++;
    if (*readCounter == 1) //first reader needs to write lock
    {
        result = WaitForSingleObject(writeMutex, FIVE_SECONDS);
        if (result == WAIT_FAILED)
        {
            (*readCounter)--;
            ReleaseMutex(readMutex);
        }
    }
    return ReleaseMutex(readMutex) || result;
}

DWORD CrossProcessLock::release()
{
    DWORD result;

    if (lockType == LockType::Unlocked)
        return 0;
    
    if (lockType == LockType::Write)
        return ReleaseMutex(writeMutex);
    
    result = WaitForSingleObject(readMutex, FIVE_SECONDS);
    if (result == WAIT_FAILED)
        return result;

    (*readCounter)--;
    if (*readCounter == 0)
    {
        result = ReleaseMutex(writeMutex);
        if (result) //on failed release, restore counter
            (*readCounter)++;
    }
    return ReleaseMutex(readMutex) || result;
}

LockType CrossProcessLock::getLockType()
{
    return lockType;
}