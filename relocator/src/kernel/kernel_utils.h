#ifndef __KERNEL_UTILS_H_
#define __KERNEL_UTILS_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "kernel_defs.h"

extern void KernelCopyData(uint32_t dst, uint32_t src, uint32_t len);

void kern_write(void *addr, uint32_t value);

uint32_t kern_read(const void *addr);

void KernelWrite(uint32_t addr, const void *data, uint32_t length);

void KernelWriteU32(uint32_t addr, uint32_t value);

void kernelInitialize();

#ifdef __cplusplus
}
#endif

#endif // __KERNEL_UTILS_H_
