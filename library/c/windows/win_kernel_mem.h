/*
 * Copyright (c) 2017 Juniper Networks, Inc. All rights reserved.
 */

//
// win_kernel_mem.h
//
// Memory management functions for Windows Drivers
//

#include <Ndis.h>

#define SxExtAllocationSandeshTag 'DNAS'

static void * win_kmalloc(unsigned int size) {
    void *mem = ExAllocatePoolWithTag(NonPagedPoolNx, size+sizeof(unsigned int), SxExtAllocationSandeshTag);
    if (!mem)
        return NULL;
    *(unsigned int*)mem = size;
    mem = (char*)mem + sizeof(unsigned int);
    return mem;
}

static void * win_kzalloc(unsigned int size) {
    void *mem = win_kmalloc(size);
    if (!mem)
        return NULL;
    RtlZeroMemory(mem, size);

    return mem;
}

static void win_kfree(void *mem) {
    if (mem) {
        mem = (char*)mem - sizeof(unsigned int);
        ExFreePoolWithTag(mem, SxExtAllocationSandeshTag);
    }
}

static void * win_krealloc(void *mem, unsigned int size) {
    unsigned int old_size;
    void *mem_tmp = win_kmalloc(size);
    if (!mem)
        return NULL;
    old_size = *(unsigned int*)((char*)mem - sizeof(unsigned int));
    RtlCopyMemory(mem_tmp, mem, old_size);
    win_kfree(mem);
    return mem_tmp;
}
