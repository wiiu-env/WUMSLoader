/* coreinit */
IMPORT_BEGIN(coreinit);

IMPORT(OSDynLoad_Acquire);
IMPORT(OSDynLoad_FindExport);
IMPORT(DCFlushRange);
IMPORT(ICInvalidateRange);
IMPORT(OSGetSystemInfo);
IMPORT(MEMGetSizeForMBlockExpHeap);
IMPORT(OSSleepTicks);
IMPORT(OSFatal);

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
