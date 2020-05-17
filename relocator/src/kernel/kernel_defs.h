#ifndef __KERNEL_DEFS_H_
#define __KERNEL_DEFS_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define KERN_SYSCALL_TBL1 0xFFE84C70 //Unknown
#define KERN_SYSCALL_TBL2 0xFFE85070 //Games
#define KERN_SYSCALL_TBL3 0xFFE85470 //Loader
#define KERN_SYSCALL_TBL4 0xFFEAAA60 //Home menu
#define KERN_SYSCALL_TBL5 0xFFEAAE60 //Browser

#ifdef __cplusplus
}
#endif

#endif // __KERNEL_DEFS_H_
