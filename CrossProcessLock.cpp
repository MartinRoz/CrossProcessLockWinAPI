#include "CrossProcessLock.hpp"
#include <stdexcept>
#include <iostream>

#define FIVE_SECONDS 5000

CrossProcessLock::CrossProcessLock(const std::wstring& lockName)
{
    readCounter = NULL;
    DWORD mutexStatus = 0;
    std::wstring writeMutexName = lockName + L"_write";
    std::wstring readMutexName = lockName + L"_read";
    std::wstring sharedMemoryName = lockName + L"_data";

    lockType = LockType::Unlocked;

    /* get or create write mutex */
    writeMutex.setHandle(CreateMutexW(NULL, FALSE, writeMutexName.c_str()));
    if (writeMutex.getHandle() == NULL)
        throw std::runtime_error("Failed to create mutex.");

    /* get or create read mutex */
    readMutex.setHandle(CreateMutexW(NULL, TRUE, readMutexName.c_str()));
    if (readMutex.getHandle() == NULL)
        throw std::runtime_error("Failed to create mutex.");
    mutexStatus = GetLastError();

    /* get or create shared data */
    sharedMemory.setHandle(CreateFileMappingW(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, sizeof(int), sharedMemoryName.c_str()));
    if (sharedMemory.getHandle() == NULL)
        throw std::runtime_error("Failed to create shared memory.");

    readCounter = (int*)MapViewOfFile(sharedMemory.getHandle(), FILE_MAP_ALL_ACCESS, 0, 0, sizeof(int));
    if (readCounter == NULL)
        throw std::runtime_error("Failed to map view of shared memory.");

    /* if the mutex was first time created, init data */
    if (mutexStatus != ERROR_ALREADY_EXISTS)
        *readCounter = 0;
    ReleaseMutex(readMutex.getHandle());
}

CrossProcessLock::~CrossProcessLock()
{
    if(readCounter != NULL)
        UnmapViewOfFile(readCounter);     
}

DWORD CrossProcessLock::lock(LockType lockType) 
{
    DWORD result = 0;

    if (lockType == LockType::Write)
    {
        result = WaitForSingleObject(writeMutex.getHandle(), FIVE_SECONDS);
        if(result == WAIT_FAILED || result == WAIT_TIMEOUT)
            return 1;
        //on success
        this->lockType = lockType;
        return 0;
    }
    
    //on read lock
    result = WaitForSingleObject(readMutex.getHandle(), FIVE_SECONDS);
    if (result == WAIT_FAILED || result == WAIT_TIMEOUT)
        return 1;
    
    (*readCounter)++;
    std::cout << "readCounter: " << *readCounter << std::endl;
    if (*readCounter == 1) //first reader needs to write lock
    {
        result = WaitForSingleObject(writeMutex.getHandle(), FIVE_SECONDS);
        if (result == WAIT_FAILED || result == WAIT_TIMEOUT)
            (*readCounter)--;
    }
    ReleaseMutex(readMutex.getHandle());
    if(result == WAIT_FAILED || result == WAIT_TIMEOUT)
        return 1;
    
    //on success
    this->lockType = lockType;
    return 0;
}

DWORD CrossProcessLock::release()
{
    DWORD result = 0;

    if (lockType == LockType::Unlocked)
        return 0;

    if (lockType == LockType::Write)
    {
        if (ReleaseMutex(writeMutex.getHandle()))
            return 1;
        lockType = LockType::Unlocked;
        return 0;
    }
    
    result = WaitForSingleObject(readMutex.getHandle(), FIVE_SECONDS);
    if (result == WAIT_FAILED || result == WAIT_TIMEOUT)
        return 1;

    (*readCounter)--;
    if (*readCounter == 0)
    {
        result = ReleaseMutex(writeMutex.getHandle());
        if (result) //on failed release, restore counter
            (*readCounter)++;
    }
    result = ReleaseMutex(readMutex.getHandle()) || result;
    if(result)
        return 1;
    lockType = LockType::Unlocked;
    return 0;
}

LockType CrossProcessLock::getLockType()
{
    return lockType;
}