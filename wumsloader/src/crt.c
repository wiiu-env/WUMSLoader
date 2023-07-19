#include <coreinit/debug.h>
#include <coreinit/thread.h>

void __init_wut_malloc();

void __init_wut_newlib();

void __init_wut_stdcpp();

void __init_wut_devoptab();

void __attribute__((weak)) __init_wut_socket();

void __fini_wut_malloc();

void __fini_wut_newlib();

void __fini_wut_stdcpp();

void __fini_wut_devoptab();

void __attribute__((weak)) __fini_wut_socket();

void __attribute__((weak))
init_wut() {
    __init_wut_malloc();
    __init_wut_newlib();
    __init_wut_stdcpp();
    __init_wut_devoptab();
    if (&__init_wut_socket) __init_wut_socket();
}

void __attribute__((weak))
fini_wut() {
    __fini_wut_devoptab();
    __fini_wut_stdcpp();
    __fini_wut_newlib();
    __fini_wut_malloc();
}

typedef enum __wut_thread_specific_id {
    WUT_THREAD_SPECIFIC_0 = 0,
    WUT_THREAD_SPECIFIC_1 = 1,
} __wut_thread_specific_id;

extern void __attribute__((weak)) wut_set_thread_specific(__wut_thread_specific_id id, void *value);

void wut_set_thread_specific(__wut_thread_specific_id id, void *value) {
    OSThread *thread;
    asm volatile("lwz %0, -0x20(0)"
                 : "=r"(thread)); // OSGetCurrentThread()
    if (thread != NULL) {
        if (id == WUT_THREAD_SPECIFIC_0) {
            thread->reserved[3] = (uint32_t) value;
        } else if (id == WUT_THREAD_SPECIFIC_1) {
            thread->reserved[4] = (uint32_t) value;
        } else {
            OSReport("[WUMSLOADER] wut_set_thread_specific: invalid id\n");
            OSFatal("[WUMSLOADER] wut_set_thread_specific: invalid id");
        }
    } else {
        OSReport("[WUMSLOADER] wut_set_thread_specific: invalid thread\n");
        OSFatal("[WUMSLOADER] wut_set_thread_specific: invalid thread");
    }
}

extern void *__attribute__((weak)) wut_get_thread_specific(__wut_thread_specific_id id);

void *wut_get_thread_specific(__wut_thread_specific_id id) {
    OSThread *thread;
    asm volatile("lwz %0, -0x20(0)"
                 : "=r"(thread)); // OSGetCurrentThread()
    if (thread != NULL) {
        if (id == WUT_THREAD_SPECIFIC_0) {
            return (void *) thread->reserved[3];
        } else if (id == WUT_THREAD_SPECIFIC_1) {
            return (void *) thread->reserved[4];
        } else {
            OSReport("[WUMSLOADER] wut_get_thread_specific: invalid id\n");
            OSFatal("[WUMSLOADER] wut_get_thread_specific: invalid id");
        }
    } else {
        OSReport("[WUMSLOADER] wut_get_thread_specific: invalid thread\n");
        OSFatal("[WUMSLOADER] wut_get_thread_specific: invalid thread");
    }
    return NULL;
}