/****************************************************************************
 * Copyright (C) 2015 Dimok
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 ****************************************************************************/
#include <coreinit/memexpheap.h>
#include <coreinit/memdefaultheap.h>
#include <coreinit/memorymap.h>
#include <coreinit/cache.h>
#include <malloc.h>
#include <string.h>
#include <errno.h>
#include "logger.h"

extern MEMHeapHandle gHeapHandle;

void *MEMAllocSafe(uint32_t size, uint32_t align) {
    void *res = nullptr;
    MEMHeapHandle heapHandle = gHeapHandle;
    MEMExpHeap *heap = (MEMExpHeap *) heapHandle;
    OSUninterruptibleSpinLock_Acquire(&heap->header.lock);
    res = MEMAllocFromExpHeapEx(heapHandle, size, align);
    auto cur = heap->usedList.head;
    while (cur != nullptr) {
        DCFlushRange(cur, sizeof(MEMExpHeapBlock));
        cur = cur->next;
    }
    cur = heap->freeList.head;
    while (cur != nullptr) {
        DCFlushRange(cur, sizeof(MEMExpHeapBlock));
        cur = cur->next;
    }
    OSUninterruptibleSpinLock_Release(&heap->header.lock);

    return res;
}


void *MemoryAlloc(uint32_t size) {
    void *res = MEMAllocSafe(size, 4);
    if (res == nullptr) {
        OSFatal_printf("Failed to MemoryAlloc %d", size);
    }
    return res;
}

void *MemoryAllocEx(uint32_t size, uint32_t align) {
    void *res = MEMAllocSafe(size, align);
    if (res == nullptr) {
        OSFatal_printf("Failed to MemoryAllocEX %d %d", size, align);
    }
    return res;
}

void MemoryFree(void *ptr) {
    if (ptr) {
        MEMFreeToExpHeap(gHeapHandle, ptr);
    } else {
        OSFatal_printf("Failed to free");
    }
}

uint32_t MEMAlloc __attribute__((__section__ (".data"))) = (uint32_t) MemoryAlloc;
uint32_t MEMAllocEx __attribute__((__section__ (".data"))) = (uint32_t) MemoryAllocEx;
uint32_t MEMFree __attribute__((__section__ (".data"))) = (uint32_t) MemoryFree;

//!-------------------------------------------------------------------------------------------
//! reent versions
//!-------------------------------------------------------------------------------------------
void *_malloc_r(struct _reent *r, size_t size) {
    void *ptr = MemoryAllocEx(size, 4);
    if (!ptr) {
        r->_errno = ENOMEM;
    }
    return ptr;
}

void *_calloc_r(struct _reent *r, size_t num, size_t size) {
    void *ptr = MemoryAllocEx(num * size, 4);
    if (ptr) {
        memset(ptr, 0, num * size);
    } else {
        r->_errno = ENOMEM;
    }

    return ptr;
}

void *_memalign_r(struct _reent *r, size_t align, size_t size) {
    return MemoryAllocEx(size, align);
}

void _free_r(struct _reent *r, void *ptr) {
    if (ptr) {
        MemoryFree(ptr);
    }
}

void *_realloc_r(struct _reent *r, void *p, size_t size) {
    void *new_ptr = MemoryAllocEx(size, 4);
    if (!new_ptr) {
        r->_errno = ENOMEM;
        return new_ptr;
    }

    if (p) {
        size_t old_size = MEMGetSizeForMBlockExpHeap(p);
        memcpy(new_ptr, p, old_size <= size ? old_size : size);
        MemoryFree(p);
    }
    return new_ptr;
}

struct mallinfo _mallinfo_r(struct _reent *r) {
    struct mallinfo info = {0};
    return info;
}

void
_malloc_stats_r(struct _reent *r) {
}

int
_mallopt_r(struct _reent *r, int param, int value) {
    return 0;
}

size_t
_malloc_usable_size_r(struct _reent *r, void *ptr) {
    return MEMGetSizeForMBlockExpHeap(ptr);
}

void *
_valloc_r(struct _reent *r, size_t size) {
    return MemoryAllocEx(size, OS_PAGE_SIZE);
}

void *
_pvalloc_r(struct _reent *r, size_t size) {
    return MemoryAllocEx((size + (OS_PAGE_SIZE - 1)) & ~(OS_PAGE_SIZE - 1), OS_PAGE_SIZE);
}

int
_malloc_trim_r(struct _reent *r, size_t pad) {
    return 0;
}
