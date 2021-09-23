#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include "logger.h"
#include <coreinit/debug.h>
#include <coreinit/systeminfo.h>
#include <coreinit/thread.h>

static int log_socket __attribute__((section(".data"))) = -1;
static struct sockaddr_in connect_addr __attribute__((section(".data")));
static volatile int log_lock __attribute__((section(".data"))) = 0;

#define SOL_SOCKET      -1


#define SOCK_DGRAM      2


#define INADDR_ANY       0
#define INADDR_BROADCAST 0xFFFFFFFF

#define PF_UNSPEC       0
#define PF_INET         2
#define PF_INET6        23

#define AF_UNSPEC       PF_UNSPEC
#define AF_INET         PF_INET
#define AF_INET6        PF_INET6

#define IPPROTO_UDP     17

#define SO_BROADCAST    0x0020      // broadcast

typedef uint16_t sa_family_t;

struct in_addr {
    unsigned int s_addr;
};

struct sockaddr {
    sa_family_t sa_family;
    char sa_data[];
};

struct sockaddr_in {
    unsigned short sin_family;
    unsigned short sin_port;
    struct in_addr sin_addr;
    char sin_zero[8];
};

typedef uint32_t socklen_t;

extern int setsockopt(int sockfd, int level, int optname, const void *optval, socklen_t optlen);

extern int socket(int domain, int type, int protocol);

extern int socketclose(int sockfd);

extern int sendto(int sockfd, const void *buf, size_t len, int flags, const struct sockaddr *dest_addr, socklen_t addrlen);

extern uint32_t htonl(uint32_t val);

void log_init() {
    int broadcastEnable = 1;
    log_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (log_socket < 0)
        return;

    setsockopt(log_socket, SOL_SOCKET, SO_BROADCAST, &broadcastEnable, sizeof(broadcastEnable));

    memset(&connect_addr, 0, sizeof(struct sockaddr_in));
    connect_addr.sin_family = AF_INET;
    connect_addr.sin_port = 4405;
    connect_addr.sin_addr.s_addr = htonl(INADDR_BROADCAST);
}

void log_deinit() {
    if (log_socket < 0) {
        return;
    }
    if (socketclose(log_socket) != 0) {
        return;
    }
}

void log_print(const char *str) {
    // socket is always 0 initially as it is in the BSS
    if (log_socket < 0) {
        return;
    }

    while (log_lock) {
        OSSleepTicks(OSMicrosecondsToTicks(1000));
    }
    log_lock = 1;

    int len = strlen(str);
    int ret;
    while (len > 0) {
        int block = len < 1400 ? len : 1400; // take max 1400 bytes per UDP packet
        ret = sendto(log_socket, str, block, 0, (struct sockaddr *) &connect_addr, sizeof(struct sockaddr_in));
        if (ret < 0)
            break;

        len -= ret;
        str += ret;
    }

    log_lock = 0;
}

void OSFatal_printf(const char *format, ...) {
    char tmp[512];
    tmp[0] = 0;
    va_list va;
    va_start(va, format);
    if ((vsprintf(tmp, format, va) >= 0)) {
        OSFatal(tmp);
    }
    va_end(va);
}

void log_printf(const char *format, ...) {
    if (log_socket < 0) {
        return;
    }

    char tmp[512];
    tmp[0] = 0;

    va_list va;
    va_start(va, format);
    if ((vsprintf(tmp, format, va) >= 0)) {
        log_print(tmp);
    }
    va_end(va);
}

