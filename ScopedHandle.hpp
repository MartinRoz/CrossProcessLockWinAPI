#include <windows.h>

class ScopedHandle
{
    private:
        HANDLE handle;
    public:
        ScopedHandle();
        ScopedHandle(HANDLE handle);
        ~ScopedHandle();

        void setHandle(HANDLE handle);
        void closeHandle();
        HANDLE getHandle();
};