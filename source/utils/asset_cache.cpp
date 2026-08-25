#include "asset_cache.h"

#include <cstdlib>
#include <cstring>

#include <psp2/kernel/processmgr.h>

#define MAX_PATH_LEN 256

namespace {
    struct CacheEntry {
        CachedAsset pub;
        char        path[MAX_PATH_LEN] = {0};
        uint64_t    lastAccessUs = 0;
        int         refcount = 0;
        bool        inUse = false;
    };

    CacheEntry *g_entries = nullptr;
    int         g_maxEntries = 0;
    size_t      g_maxTotalBytes = 0;
    size_t      g_totalBytes = 0;
    uint64_t    g_ttlUs = 0;

    uint64_t now_us() {
        return sceKernelGetProcessTimeWide();
    }

    CacheEntry *find_entry(const char *path) {
        for (int i = 0; i < g_maxEntries; i++) {
            if (g_entries[i].inUse && strcmp(g_entries[i].path, path) == 0) {
                return &g_entries[i];
            }
        }
        return nullptr;
    }

    void evict_entry(CacheEntry *e) {
        if (e->pub.data) {
            free(e->pub.data);
            g_totalBytes -= e->pub.size;
        }
        e->pub.data = nullptr;
        e->pub.size = 0;
        e->refcount = 0;
        e->inUse = false;
        e->path[0] = '\0';
    }

    bool evict_lru() {
        CacheEntry *oldest = nullptr;
        for (int i = 0; i < g_maxEntries; i++) {
            CacheEntry *e = &g_entries[i];
            if (!e->inUse || e->refcount > 0) continue;
            if (!oldest || e->lastAccessUs < oldest->lastAccessUs) oldest = e;
        }
        if (oldest) {
            evict_entry(oldest);
            return true;
        }
        return false;
    }

    CacheEntry *find_free_slot() {
        for (int i = 0; i < g_maxEntries; i++) {
            if (!g_entries[i].inUse) return &g_entries[i];
        }
        return nullptr;
    }

}

void asset_cache_init(int maxEntries, size_t maxTotalBytes, uint64_t ttlUs) {
    g_entries = new CacheEntry[maxEntries];
    g_maxEntries = maxEntries;
    g_maxTotalBytes = maxTotalBytes;
    g_totalBytes = 0;
    g_ttlUs = ttlUs;
}

CachedAsset *asset_cache_acquire(const char *path) {
    CacheEntry *e = find_entry(path);
    if (!e) return nullptr;

    e->lastAccessUs = now_us();
    e->refcount++;
    return &e->pub;
}

CachedAsset *asset_cache_insert(const char *path, void *data, size_t size) {
    if (find_entry(path)) {
        return nullptr;
    }

    while (g_totalBytes + size > g_maxTotalBytes) {
        if (!evict_lru()) break;
    }

    CacheEntry *slot = find_free_slot();
    while (!slot) {
        if (!evict_lru()) return nullptr;
        slot = find_free_slot();
    }

    strncpy(slot->path, path, MAX_PATH_LEN - 1);
    slot->path[MAX_PATH_LEN - 1] = '\0';
    slot->pub.data = data;
    slot->pub.size = size;
    slot->lastAccessUs = now_us();
    slot->refcount = 1;
    slot->inUse = true;
    g_totalBytes += size;

    return &slot->pub;
}

void asset_cache_release(CachedAsset *asset) {
    if (!asset) return;
    auto *e = reinterpret_cast<CacheEntry *>(asset);
    if (e->refcount > 0) e->refcount--;
    e->lastAccessUs = now_us();
}

void asset_cache_gc() {
    if (g_ttlUs == 0) return;
    uint64_t t = now_us();
    for (int i = 0; i < g_maxEntries; i++) {
        CacheEntry *e = &g_entries[i];
        if (e->inUse && e->refcount == 0 && (t - e->lastAccessUs) > g_ttlUs) {
            evict_entry(e);
        }
    }
}

void asset_cache_clear() {
    for (int i = 0; i < g_maxEntries; i++) {
        if (g_entries[i].inUse && g_entries[i].refcount == 0) {
            evict_entry(&g_entries[i]);
        }
    }
}
