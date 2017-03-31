#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <inttypes.h>
#include <string.h>

#include "disk.h"
#include "sfs.h"


/* Given a pointer to a particular disk, read entry `n` from directory dir_inode, and store its info into `dir`. 
 * Returns 0 on success or -1 on failure */
int sfs_read_dir_entry(struct sfs_disk* disk, struct sfs_inode* dir_inode, int n, struct sfs_dir_entry* dir)
{
        int dir_block_index = 0; // TODO: currently only support using the first data block in an inode
        int dir_block;  // FIXME: this should be the actual block ID to read from
        int dir_offset; // FIXME: this should be the byte offset inside the block to read from (based on n)

        // find the right offsets and read the inum, file name, and file name length from disk
        // remember, if the strlen field in the dir_entry is 0 that means it is unused
        // return 0 if this is a valid dir entry, or -1 if it is unused.

        return -1;
}

/* Create a new file entry in a directory based on info in `direntry`. 
 * Assumes there is only 1 block for data.
 * Returns index of the entry or -1 on failure */
int sfs_create_dir_entry(struct sfs_disk* disk, struct sfs_inode* dir_inode, struct sfs_dir_entry* direntry)
{
        int dir_block_index = 0; // TODO: currently only support using the first data block in an inode
        int n; // index of the new entry
        /* for example n=0 means this is the first entry in the data block
         * and n=7 is the last entry in the data block, since we can fit 8 dir entries per block (128/16)
         * and n=12 would be an entry in the second data block (once we add support for multi-block dirs!)*/
        int dir_block;  // actual block ID to write to
        int dir_offset; // byte offset inside the block to write to (based on n)

        // Read through the dir_entries currently stored on disk to find an empty one (n)
        // then figure out which block that is and what offset it is within the block

        /* Note: we need read_dir_entry to work, since this function will need to
         * read through the dir_entries in the dir_inode's data block until it finds an unused one.
         * Once we find a free entry on disk, write the values from direntry into it. */

        for(n=0; n < SFS_BLOCK_SIZE / SFS_DIR_ENTRY_SIZE; n++) {
                // Find a free entry
                struct sfs_dir_entry tmp_entry;
                int invalid;
                invalid = sfs_read_dir_entry(disk, dir_inode, n, &tmp_entry);
                if(invalid) {
                        printf("Using dir entry %d\n", n);
                        break; // can use entry n
                }
        }
        if (n == SFS_BLOCK_SIZE/SFS_DIR_ENTRY_SIZE){
            printf("Failure, no available directory entry\n");
            return -1;
        }
        dir_block = dir_inode->block[dir_block_index];
        dir_offset = (n * SFS_DIR_ENTRY_SIZE) % SFS_BLOCK_SIZE;
        disk_write(disk->data, dir_block, dir_offset, &direntry->inum, 1);
        disk_write(disk->data, dir_block, dir_offset+1, &direntry->strlen, 1);
        disk_write(disk->data, dir_block, dir_offset+2, &direntry->name, direntry->strlen+1);
        dir_inode->size += SFS_DIR_ENTRY_SIZE;
        sfs_write_inode(disk, 0, dir_inode); // assumes writing to root dir
        return 0;
}

/* List out a directory by scanning all of its data blocks for valid dir_entries. */
void sfs_ls_dir(struct sfs_disk* disk, struct sfs_inode* dir_inode)
{
        if(dir_inode->type != 2) {
                printf("ERROR: tried to list a non-directory inode\n");
                return;
        }
        printf("          NAME     TYPE       SIZE         BLOCK LIST\n");
        int max_used_entries = dir_inode->used_blocks * SFS_BLOCK_SIZE / SFS_DIR_ENTRY_SIZE;
        for(int n=0; n < max_used_entries; n++) {
                struct sfs_dir_entry dir;
                int invalid;
                invalid = sfs_read_dir_entry(disk, dir_inode, n, &dir);
                if(invalid == 0) {
                        sfs_print_dir_entry(disk, &dir);
                }
        }
}

/* Print the file or directory name and data from its inode */
void sfs_print_dir_entry(struct sfs_disk* disk, struct sfs_dir_entry* dir) {
        printf(" %16s", dir->name);
        struct sfs_inode inode;
        sfs_read_inode(disk, dir->inum, &inode);
        sfs_print_inode(&inode);
}

/* Given a file/directory path, find the directory entry for it and fill in `entry`.
 * Only searches root directory. Returns 0, or -1 on failure */
int sfs_find_dir_entry(struct sfs_disk* disk, char* filename, struct sfs_dir_entry* entry)
{
        int n;
        struct sfs_inode* root_dir = &disk->root_dir_inode; // TODO: will need to change this to support nested directories

        /* FIXME: need to read each dir entry in the directory from disk and see if it has
         * the same filename. If it does, read all of its data into `entry` and return 0.
         * If the file can't be found, return -1. */

         //TODO: needs to work with files not in root directory

        return -1;

}
