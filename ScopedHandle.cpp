#include "ScopedHandle.hpp"
#include <iostream>

ScopedHandle::ScopedHandle()
{
    this->handle = NULL;
}

ScopedHandle::ScopedHandle(HANDLE handle)
{
    this->handle = handle;   
}

ScopedHandle::~ScopedHandle()
{
    closeHandle();
}

void ScopedHandle::setHandle(HANDLE handle)
{
    this->handle = handle;
}

void ScopedHandle::closeHandle()
{
    if(handle == NULL)
        return;
    
    CloseHandle(handle);
    handle = NULL;

}
HANDLE ScopedHandle::getHandle()
{
    return handle;
}