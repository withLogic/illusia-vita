/*
 * Copyright (C) 2021      Andy Nguyen
 * Copyright (C) 2022      Rinnegatamante
 * Copyright (C) 2022-2024 Volodymyr Atamanenko
 *
 * This software may be modified and distributed under the terms
 * of the MIT license. See the LICENSE file for details.
 */

/**
 * @file  io.h
 * @brief Wrappers and implementations for some of the IO functions.
 */

#ifndef SOLOADER_IO_H
#define SOLOADER_IO_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <sys/dirent.h>
#include <sys/syslimits.h>
#include <sys/fcntl.h>

#ifndef PATH_MAX
#define PATH_MAX 1024
#endif

#ifndef DT_DIR
#define DT_UNKNOWN 0
#define DT_FIFO 1
#define DT_CHR 2
#define DT_DIR 4
#define DT_BLK 6
#define DT_REG 8
#define DT_LNK 10
#define DT_SOCK 12
#define DT_WHT 14
#endif

typedef struct __attribute__((__packed__)) stat64_bionic {
    /*
    this fixes the ability to load the save files and options file. 
    fix was discovered by Claude for the Advena port by MetalSyntax 
    and used here.
    */
    uint64_t st_dev;         // 0x00 (8 bytes)
    uint32_t __pad0;         // 0x08 (4 bytes)
    uint32_t __st_ino;       // 0x0C (4 bytes)
    uint32_t st_mode;        // 0x10 (4 bytes)
    uint32_t st_nlink;       // 0x14 (4 bytes) - explicit uint32_t (newlib nlink_t is 2 bytes!)
    uint32_t st_uid;         // 0x18 (4 bytes) - explicit uint32_t (newlib uid_t is 2 bytes!)
    uint32_t st_gid;         // 0x1C (4 bytes) - explicit uint32_t (newlib gid_t is 2 bytes!)
    uint64_t st_rdev;        // 0x20 (8 bytes)
    uint32_t __pad3;         // 0x28 (4 bytes)
    uint32_t __pad4;         // 0x2C (4 bytes)
    int64_t  st_size;        // 0x30 (8 bytes) - offset 0x30 (48 bytes) for Android Bionic 32-bit ABI
    uint32_t st_blksize;     // 0x38 (4 bytes)
    uint32_t __pad5;         // 0x3C (4 bytes)
    uint64_t st_blocks;      // 0x40 (8 bytes)
    uint32_t st_atime;       // 0x48 (4 bytes)
    uint32_t st_atime_nsec;  // 0x4C (4 bytes)
    uint32_t st_mtime;       // 0x50 (4 bytes)
    uint32_t st_mtime_nsec;  // 0x54 (4 bytes)
    uint32_t st_ctime;       // 0x58 (4 bytes)
    uint32_t st_ctime_nsec;  // 0x5C (4 bytes)
    uint64_t st_ino;         // 0x60 (8 bytes)
} stat64_bionic;

typedef struct __attribute__((__packed__)) dirent64_bionic {
    int16_t d_ino; // 2 bytes // offset 0x0
    int64_t d_off; // 8 bytes // offset 0x2
    uint64_t d_reclen; // 8 bytes // 0xA
    unsigned char d_type; // 1 byte // offset 0x12
    char d_name[256]; // 256 bytes // offset 0x13
} dirent64_bionic;

int open_soloader(const char * path, int oflag, ...);

FILE * fopen_soloader(const char * filename, const char * mode);

DIR *opendir_soloader(char *name);

int stat_soloader(const char * path, stat64_bionic * buf);

int fstat_soloader(int fd, stat64_bionic * buf);

struct dirent64_bionic * readdir_soloader(DIR *dir);

int readdir_r_soloader(DIR * dirp, dirent64_bionic * entry,
                       dirent64_bionic ** result);

int close_soloader(int fd);

int fclose_soloader(FILE *f);

int closedir_soloader(DIR *dir);

int fcntl_soloader(int fd, int cmd, ...);

int ioctl_soloader(int fd, int request, ... /* arg */);

int fsync_soloader(int fd);

#ifdef __cplusplus
};
#endif

#endif // SOLOADER_IO_H
