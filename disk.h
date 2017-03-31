#ifndef DISK_H
#define DISK_H

#include "sfs.h"

//#define VERBOSE_DISK

/* Read from an in-memory "disk".
 * inputs:
 *   disk = memory array representing disk
 *   block = index of the block to read from
 *   offset = byte offset within the block to start read
 *   dst = pointer where disk data will be read into
 *   num_bytes = length of data to read
 */
static inline void
disk_read(char* disk, int block, int offset, void* dst, int num_bytes) {
        #ifdef VERBOSE_DISK
                printf("LOG: read block %d offset %d size %d\n", block, offset, num_bytes);
        #endif
        memcpy(dst, &disk[block*SFS_BLOCK_SIZE + offset], num_bytes);
}
/* Write to an in-memory "disk".
 * inputs:
 *   disk = memory array representing disk
 *   block = index of the block to write to
 *   offset = byte offset within the block to start write
 *   dst = pointer to data to be written to disk
 *   num_bytes = length of data to read
 */
static inline void
disk_write(char* disk, int block, int offset, void* src, int num_bytes) {
        #ifdef VERBOSE_DISK
                printf("LOG: write block %d offset %d size %d\n", block, offset, num_bytes);
        #endif
        memcpy(&disk[block*SFS_BLOCK_SIZE + offset], src, num_bytes);
}

#endif
