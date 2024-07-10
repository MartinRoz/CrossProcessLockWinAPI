## CrossProccessLock

# Shared Memory
* file handle
* counter for number of lock objects across the proccesses
* readCounter for number of lock object currently reading
* isWrite boolean for if a lock object is currently writing

# on construct
* if sharedMemory does not exist init it
* if sharedMemory exists
    * increase counter by 1
    * get fileHandle?

# on deconstruct
* release current lock
* decrease counter by 1
* if counter is 0
    * delete shared memory 
    
# on lock
* if desired lock is write
    * wait untill isWrite is false and readCounter is 0
    * change isWrite to true

* if desired lock is read
    * wait untill isWrite is false
    * increase readCounter by 1

# on release
* if lock type was read
* if lock type was write
