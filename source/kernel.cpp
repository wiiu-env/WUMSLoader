#include "kernel.h"
#include "../wumsloader/src/globals.h"
#include "ElfUtils.h"
#include "wumsloader_elf.h"
#include <coreinit/cache.h>
#include <coreinit/memorymap.h>

extern "C" void SCKernelCopyData(uint32_t addr, uint32_t src, uint32_t len);
extern "C" void SC_KernelCopyData(uint32_t addr, uint32_t src, uint32_t len);

void SetupWUMSLoader() {
    kern_write((void *) (KERN_SYSCALL_TBL_1 + (0x25 * 4)), (unsigned int) SCKernelCopyData);
    kern_write((void *) (KERN_SYSCALL_TBL_2 + (0x25 * 4)), (unsigned int) SCKernelCopyData);
    kern_write((void *) (KERN_SYSCALL_TBL_3 + (0x25 * 4)), (unsigned int) SCKernelCopyData);
    kern_write((void *) (KERN_SYSCALL_TBL_4 + (0x25 * 4)), (unsigned int) SCKernelCopyData);
    kern_write((void *) (KERN_SYSCALL_TBL_5 + (0x25 * 4)), (unsigned int) SCKernelCopyData);

    uint32_t entryPoint = load_loader_elf(0, (char *) wumsloader_elf, wumsloader_elf_size);

    unsigned int repl_addr = ADDRESS_main_entry_hook;
    KernelWriteU32(repl_addr, 0x48000003 | entryPoint);
    DCFlushRange((void *) repl_addr, 4);
    ICInvalidateRange((void *) (repl_addr), 4);

    // We call the mainhook (wumloader) once with a MAGIC number to set up the WUMS Modules but not actually call the original main function
    ((int (*)(int, char **))(entryPoint))(WUMS_LOADER_SETUP_MAGIC_WORD, nullptr);
}

void KernelWriteU32(uint32_t addr, uint32_t value) {

    ICInvalidateRange(&value, 4);
    DCFlushRange(&value, 4);

    auto dst = (uint32_t) OSEffectiveToPhysical(addr);
    auto src = (uint32_t) OSEffectiveToPhysical((uint32_t) &value);

    SC_KernelCopyData(dst, src, 4);

    DCFlushRange((void *) addr, 4);
    ICInvalidateRange((void *) addr, 4);
}

/* Write a 32-bit word with kernel permissions */
void __attribute__((noinline)) kern_write(void *addr, uint32_t value) {
    asm volatile(
            "li 3,1\n"
            "li 4,0\n"
            "mr 5,%1\n"
            "li 6,0\n"
            "li 7,0\n"
            "lis 8,1\n"
            "mr 9,%0\n"
            "mr %1,1\n"
            "li 0,0x3500\n"
            "sc\n"
            "nop\n"
            "mr 1,%1\n"
            :
            : "r"(addr), "r"(value)
            : "memory", "ctr", "lr", "0", "3", "4", "5", "6", "7", "8", "9", "10",
              "11", "12");
}
