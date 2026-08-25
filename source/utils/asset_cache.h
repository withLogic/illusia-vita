#ifndef ASSET_CACHE_H
#define ASSET_CACHE_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct CachedAsset {
    void   *data;
    size_t  size;
} CachedAsset;

void asset_cache_init(int maxEntries, size_t maxTotalBytes, uint64_t ttlUs);

CachedAsset *asset_cache_acquire(const char *path);

CachedAsset *asset_cache_insert(const char *path, void *data, size_t size);

void asset_cache_release(CachedAsset *asset);

void asset_cache_gc(void);

void asset_cache_clear(void);

#ifdef __cplusplus
}
#endif

#endif