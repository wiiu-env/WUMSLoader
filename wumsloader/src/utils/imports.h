/* coreinit */
IMPORT_BEGIN(coreinit);

IMPORT(OSDynLoad_Acquire);
IMPORT(OSDynLoad_FindExport);
IMPORT(OSDynLoad_IsModuleLoaded);
IMPORT(DCFlushRange);
IMPORT(ICInvalidateRange);
IMPORT(OSGetSystemInfo);
IMPORT(MEMGetSizeForMBlockExpHeap);
IMPORT(OSSleepTicks);
IMPORT(OSEffectiveToPhysical);
IMPORT(OSFatal);
IMPORT(MEMFreeToExpHeap);
IMPORT(MEMAllocFromExpHeapEx);
IMPORT(MEMGetAllocatableSizeForExpHeapEx);
IMPORT(MEMCreateExpHeapEx);
IMPORT(OSGetUPID);
IMPORT(OSUninterruptibleSpinLock_Acquire);
IMPORT(OSUninterruptibleSpinLock_Release);
IMPORT(OSReport);
IMPORT(MEMGetTotalFreeSizeForExpHeap);
IMPORT(OSMemoryBarrier);
IMPORT(OSInitMutex);
IMPORT(OSLockMutex);
IMPORT(OSUnlockMutex);
IMPORT(OSCompareAndSwapAtomicEx);
IMPORT(OSCompareAndSwapAtomic);
IMPORT(OSGetThreadSpecific);
IMPORT(OSSetThreadSpecific);
IMPORT(OSInitCond);
IMPORT(OSWaitCond);
IMPORT(OSSignalCond);
IMPORT(MEMAllocFromDefaultHeapEx);
IMPORT(MEMFreeToDefaultHeap);
IMPORT(OSSwapAtomic);

IMPORT(FSTimeToCalendarTime);
IMPORT(FSInit);
IMPORT(FSShutdown);
IMPORT(FSAddClient);
IMPORT(FSAddClientEx);
IMPORT(FSDelClient);
IMPORT(FSInitCmdBlock);
IMPORT(FSChangeDir);
IMPORT(FSGetFreeSpaceSize);
IMPORT(FSGetStat);
IMPORT(FSRemove);
IMPORT(FSOpenFile);
IMPORT(FSOpenFileEx);
IMPORT(FSCloseFile);
IMPORT(FSOpenDir);
IMPORT(FSMakeDir);
IMPORT(FSReadDir);
IMPORT(FSRewindDir);
IMPORT(FSCloseDir);
IMPORT(FSGetStatFile);
IMPORT(FSReadFile);
IMPORT(FSWriteFile);
IMPORT(FSSetPosFile);
IMPORT(FSFlushFile);
IMPORT(FSTruncateFile);
IMPORT(FSRename);
IMPORT(FSGetMountSource);
IMPORT(FSMount);
IMPORT(FSUnmount);
IMPORT(FSChangeMode);
IMPORT(FSGetPosFile);
IMPORT(OSTicksToCalendarTime);
IMPORT(__rplwrap_exit);
IMPORT(OSInitMutexEx);

IMPORT(FSAMakeDir);
IMPORT(FSAInit);
IMPORT(FSAAddClient);
IMPORT(FSARewindDir);
IMPORT(FSAMount);
IMPORT(FSAGetDeviceInfo);
IMPORT(FSARename);
IMPORT(FSAChangeDir);
IMPORT(FSAUnmount);
IMPORT(FSADelClient);
IMPORT(FSAChangeMode);
IMPORT(FSAReadDir);
IMPORT(FSAOpenDir);
IMPORT(FSACloseDir);
IMPORT(FSAFlushFile);
IMPORT(FSAOpenFileEx);
IMPORT(FSACloseFile);
IMPORT(FSAGetStatFile);
IMPORT(FSAGetFreeSpaceSize);
IMPORT(FSASetPosFile);
IMPORT(FSATruncateFile);
IMPORT(FSARemove);
IMPORT(FSAReadFile);
IMPORT(FSAWriteFile);
IMPORT(FSAGetStat);
IMPORT(FSAGetStatusStr);

IMPORT_END();

/* nsysnet */
IMPORT_BEGIN(nsysnet);

IMPORT(socket_lib_init);
IMPORT(getaddrinfo);
IMPORT(freeaddrinfo);
IMPORT(getnameinfo);
IMPORT(inet_ntoa);
IMPORT(inet_ntop);
IMPORT(inet_aton);
IMPORT(inet_pton);
IMPORT(ntohl);
IMPORT(ntohs);
IMPORT(htonl);
IMPORT(htons);
IMPORT(accept);
IMPORT(bind);
IMPORT(socketclose);
IMPORT(connect);
IMPORT(getpeername);
IMPORT(getsockname);
IMPORT(getsockopt);
IMPORT(listen);
IMPORT(recv);
IMPORT(recvfrom);
IMPORT(send);
IMPORT(sendto);
IMPORT(setsockopt);
IMPORT(shutdown);
IMPORT(socket);
IMPORT(select);
IMPORT(socketlasterr);

IMPORT_END();
