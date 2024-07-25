#include "CrossProcessLock.hpp"
#include <stdexcept>

CrossProcessLock::CrossProcessLock(std::wstring lockName)
{
    DWORD mutexStatus;
    std::wstring writeMutexName = lockName + L"_write";
    std::wstring readMutexName = lockName + L"_read";
    std::wstring sharedMemoryName = lockName + L"_data";

    /* get or create write mutex */
    this->writeMutex = CreateMutexW(NULL, FALSE, writeMutexName.c_str());
    if (this->writeMutex == NULL)
        throw std::runtime_error("Failed to create mutex.");

    /* get or create read mutex */
    this->readMutex = CreateMutexW(NULL, TRUE, readMutexName.c_str());
    if (this->readMutex == NULL)
    {
        CloseHandle(this->writeMutex);
        throw std::runtime_error("Failed to create mutex.");
    }
    mutexStatus = GetLastError();

    /* get or create shared data */
    this->sharedMemory = CreateFileMappingW(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, sizeof(int), sharedMemoryName.c_str());
    if (this->sharedMemory == NULL)
    {
        CloseHandle(this->writeMutex);
        CloseHandle(this->readMutex);
        throw std::runtime_error("Failed to create shared memory.");
    }

    this->readCounter = (int*)MapViewOfFile(sharedMemory, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(int));
    if (this->readCounter == NULL)
    {
        CloseHandle(this->sharedMemory);
        CloseHandle(this->writeMutex);
        CloseHandle(this->readMutex);
        throw std::runtime_error("Failed to map view of shared memory.");
    }

    /* if the mutex was first time created, init data */
    if(mutexStatus != ERROR_ALREADY_EXISTS)
        *this->readCounter = 0;
    ReleaseMutex(this->readMutex);
}

CrossProcessLock::~CrossProcessLock()
{
    CloseHandle(this->writeMutex);     
    CloseHandle(this->readMutex);
    CloseHandle(this->sharedMemory);
    UnmapViewOfFile(this->readCounter);     
}

DWORD CrossProcessLock::lock(LockType lockType) 
{
    DWORD result;
    this->lockType = lockType;

    if(lockType == LockType::Write)
        return WaitForSingleObject(this->writeMutex, INFINITE);
    
    //on read lock
    result = WaitForSingleObject(this->readMutex, INFINITE);
    if(result == WAIT_FAILED)
        return result;
    
    (*this->readCounter)++;
    if(*this->readCounter == 1) //first reader needs to write lock
    {
        result = WaitForSingleObject(this->writeMutex, INFINITE);
        if(result == WAIT_FAILED)
        {
            (*this->readCounter)--;
            ReleaseMutex(this->readMutex);
        }
    }
    return ReleaseMutex(this->readMutex) || result;
}

DWORD CrossProcessLock::release()
{
    DWORD result;

    if(this->lockType == LockType::Write)
        return ReleaseMutex(this->writeMutex);
    
    result = WaitForSingleObject(this->readMutex, INFINITE);
    if(result == WAIT_FAILED)
        return result;

    (*this->readCounter)--;
    if(*this->readCounter == 0)
    {
        result = ReleaseMutex(this->writeMutex);
        if(result) //on failed release, restore counter
            (*this->readCounter)++;
    }
    return ReleaseMutex(this->readMutex) || result;
}