#include "utils/logger.h"
#include "utils/utils.h"
#include <coreinit/memdefaultheap.h>
#include <cstdint>
#include <fcntl.h>
#include <malloc.h>
#include <string>
#include <unistd.h>

int32_t LoadFileToMem(const std::string &filepath, uint8_t **inbuffer, uint32_t *size) {
    //! always initialze input
    *inbuffer = nullptr;
    if (size) {
        *size = 0;
    }

    int32_t iFd = open(filepath.c_str(), O_RDONLY);
    if (iFd < 0) {
        return -1;
    }

    uint32_t filesize = lseek(iFd, 0, SEEK_END);
    lseek(iFd, 0, SEEK_SET);

    auto *buffer = (uint8_t *) MEMAllocFromDefaultHeapEx(ROUNDUP(filesize, 0x40), 0x40);
    if (buffer == nullptr) {
        close(iFd);
        return -2;
    }

    uint32_t blocksize = 0x20000;
    uint32_t done      = 0;
    int32_t readBytes  = 0;

    while (done < filesize) {
        if (done + blocksize > filesize) {
            blocksize = filesize - done;
        }
        readBytes = read(iFd, buffer + done, blocksize);
        if (readBytes <= 0)
            break;
        done += readBytes;
    }

    ::close(iFd);

    if (done != filesize) {
        memset(buffer, 0, ROUNDUP(filesize, 0x40));
        free(buffer);
        buffer = nullptr;
        return -3;
    }

    *inbuffer = buffer;

    //! sign is optional input
    if (size) {
        *size = filesize;
    }

    return filesize;
}