/* libs/pixelflinger/codeflinger/CodeCache.cpp
**
** Copyright 2006, The Android Open Source Project
**
** Licensed under the Apache License, Version 2.0 (the "License");
** you may not use this file except in compliance with the License.
** You may obtain a copy of the License at
**
**     http://www.apache.org/licenses/LICENSE-2.0
**
** Unless required by applicable law or agreed to in writing, software
** distributed under the License is distributed on an "AS IS" BASIS,
** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
** See the License for the specific language governing permissions and
** limitations under the License.
*/

#define LOG_TAG "CodeCache"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <unistd.h>

#include <cutils/ashmem.h>
#include <log/log.h>

#include "CodeCache.h"

namespace android {

// ----------------------------------------------------------------------------

#if defined(__arm__) || defined(__aarch64__)
#include <unistd.h>
#include <errno.h>
#endif

#if defined(__mips__)
#include <asm/cachectl.h>
#include <errno.h>
#endif

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

// A dlmalloc mspace is used to manage the code cache over a mmaped region.
#define HAVE_MMAP 0
#define HAVE_MREMAP 0
#define HAVE_MORECORE 0
#define MALLOC_ALIGNMENT 16
#define MSPACES 1
#define NO_MALLINFO 1
#define ONLY_MSPACES 1
// Custom heap error handling.
#define PROCEED_ON_ERROR 0
static void heap_error(const char* msg, const char* function, void* p);
#define CORRUPTION_ERROR_ACTION(m) \
    heap_error("HEAP MEMORY CORRUPTION", __FUNCTION__, NULL)
#define USAGE_ERROR_ACTION(m,p) \
    heap_error("ARGUMENT IS INVALID HEAP ADDRESS", __FUNCTION__, p)

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wexpansion-to-defined"
#pragma GCC diagnostic ignored "-Wnull-pointer-arithmetic"
#include "../../../../external/dlmalloc/malloc.c"
#pragma GCC diagnostic pop

static void heap_error(const char* msg, const char* function, void* p) {
    ALOG(LOG_FATAL, LOG_TAG, "@@@ ABORTING: CODE FLINGER: %s IN %s addr=%p",
         msg, function, p);
    /* So that we can get a memory dump around p */
    *((int **) 0xdeadbaad) = (int *) p;
}

// ----------------------------------------------------------------------------

static void* gExecutableStore = NULL;  /* RX mapping — for execution */
static void* gWritableStore   = NULL;  /* RW mapping — for code generation */
static mspace gMspace = NULL;
const size_t kMaxCodeCacheCapacity = 1024 * 1024;

static mspace getMspace()
{
    if (gExecutableStore == NULL) {
        int fd = ashmem_create_region("CodeFlinger code cache",
                                      kMaxCodeCacheCapacity);
        LOG_ALWAYS_FATAL_IF(fd < 0,
                            "Creating code cache, ashmem_create_region "
                            "failed with error '%s'", strerror(errno));

        /* RW mapping for writing JIT code */
        gWritableStore = mmap(NULL, kMaxCodeCacheCapacity,
                              PROT_READ | PROT_WRITE,
                              MAP_SHARED, fd, 0);
        LOG_ALWAYS_FATAL_IF(gWritableStore == MAP_FAILED,
                            "Creating code cache (RW), mmap failed: '%s'",
                            strerror(errno));

        /* RX mapping of the same fd — for execution */
        gExecutableStore = mmap(NULL, kMaxCodeCacheCapacity,
                                PROT_READ | PROT_EXEC,
                                MAP_SHARED, fd, 0);
        LOG_ALWAYS_FATAL_IF(gExecutableStore == MAP_FAILED,
                            "Creating code cache (RX), mmap failed: '%s'",
                            strerror(errno));

        close(fd);

        /* Allocator runs over the WRITABLE mapping */
        gMspace = create_mspace_with_base(gWritableStore, kMaxCodeCacheCapacity,
                                          /*locked=*/ false);
        mspace_set_footprint_limit(gMspace, kMaxCodeCacheCapacity);
    }
    return gMspace;
}

Assembly::Assembly(size_t size)
    : mCount(0), mSize(0)
{
    /* Allocate from the WRITABLE store */
    mBase = (uint32_t*)mspace_malloc(getMspace(), size);
    LOG_ALWAYS_FATAL_IF(mBase == NULL,
                        "Failed to create Assembly of size %zd in executable "
                        "store of size %zd", size, kMaxCodeCacheCapacity);
    mSize = size;
}

Assembly::~Assembly()
{
    mspace_free(getMspace(), mBase);
}

void Assembly::incStrong(const void*) const
{
    mCount.fetch_add(1, std::memory_order_relaxed);
}

void Assembly::decStrong(const void*) const
{
    if (mCount.fetch_sub(1, std::memory_order_acq_rel) == 1) {
        delete this;
    }
}

ssize_t Assembly::size() const
{
    if (!mBase) return NO_MEMORY;
    return mSize;
}

uint32_t* Assembly::base() const
{
    /* Return pointer into the EXECUTABLE (RX) mapping */
    if (!mBase || !gWritableStore || !gExecutableStore) return mBase;
    ptrdiff_t offset = (char*)mBase - (char*)gWritableStore;
    return (uint32_t*)((char*)gExecutableStore + offset);
}

uint32_t* Assembly::writable_base() const
{
    /* Return pointer into the WRITABLE (RW) mapping for code generation */
    return mBase;
}

ssize_t Assembly::resize(size_t newSize)
{
    /* Resize in the WRITABLE mapping */
    mBase = (uint32_t*)mspace_realloc(getMspace(), mBase, newSize);
    LOG_ALWAYS_FATAL_IF(mBase == NULL,
                        "Failed to resize Assembly to %zd in code cache "
                        "of size %zd", newSize, kMaxCodeCacheCapacity);
    mSize = newSize;
    return size();
}

// ----------------------------------------------------------------------------

CodeCache::CodeCache(size_t size)
    : mCacheSize(size), mCacheInUse(0)
{
    pthread_mutex_init(&mLock, 0);
}

CodeCache::~CodeCache()
{
    pthread_mutex_destroy(&mLock);
}

sp<Assembly> CodeCache::lookup(const AssemblyKeyBase& keyBase) const
{
    pthread_mutex_lock(&mLock);
    sp<Assembly> r;
    ssize_t index = mCacheData.indexOfKey(key_t(keyBase));
    if (index >= 0) {
        const cache_entry_t& e = mCacheData.valueAt(index);
        e.when = mWhen++;
        r = e.entry;
    }
    pthread_mutex_unlock(&mLock);
    return r;
}

int CodeCache::cache(  const AssemblyKeyBase& keyBase,
                            const sp<Assembly>& assembly)
{
    pthread_mutex_lock(&mLock);

    const ssize_t assemblySize = assembly->size();
    while (mCacheInUse + assemblySize > mCacheSize) {
        // evict the LRU
        size_t lru = 0;
        size_t count = mCacheData.size();
        for (size_t i=0 ; i<count ; i++) {
            const cache_entry_t& e = mCacheData.valueAt(i);
            if (e.when < mCacheData.valueAt(lru).when) {
                lru = i;
            }
        }
        const cache_entry_t& e = mCacheData.valueAt(lru);
        mCacheInUse -= e.entry->size();
        mCacheData.removeItemsAt(lru);
    }

    ssize_t err = mCacheData.add(key_t(keyBase), cache_entry_t(assembly, mWhen));
    if (err >= 0) {
        mCacheInUse += assemblySize;
        mWhen++;
        /* Flush D-cache on the WRITABLE mapping so RX mapping sees the code */
        char* wbase = reinterpret_cast<char*>(assembly->writable_base());
        char* wcurr = wbase + assembly->size();
        __builtin___clear_cache(wbase, wcurr);
        /* Also flush I-cache on the EXECUTABLE mapping */
        char* xbase = reinterpret_cast<char*>(assembly->base());
        char* xcurr = xbase + assembly->size();
        __builtin___clear_cache(xbase, xcurr);
    }

    pthread_mutex_unlock(&mLock);
    return err;
}

// ----------------------------------------------------------------------------

}; // namespace android