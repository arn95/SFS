#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <inttypes.h>
#include <string.h>

#include "disk.h"
#include "sfs.h"

/* Close a file and zero out the open file struct. Returns 0 with success, or -1 on failure. */
int sfs_close(struct sfs_disk* disk, int filedes)
{
        struct sfs_open_file* file = &disk->open_list[filedes];
        if(filedes < 0 || filedes >= SFS_MAX_OPEN_FILES || disk->open_list[filedes].used == 0) {
                printf("ERROR: tried to close invalid file descriptor!\n");
                return -1;
        }
        memset(file, 0, sizeof(struct sfs_open_file));
        disk->open_files--;
        return 0;
}

/* Open a file and return a file descriptor, or -1 on failure.
 * Fill in the file descriptor offset and inode.
 * If create_flag=1, create a new file, else look for an existing file. */
int sfs_open(struct sfs_disk* disk, char* filename, int create_flag)
{
        int fp = -1;
        struct sfs_open_file* file;
        struct sfs_inode* inode; // pointer to inode inside open_file struct
        if(disk->open_files >= SFS_MAX_OPEN_FILES) {
                printf("ERROR: too many files open!\n");
                return -1;
        }
        // find a free file descriptor
        for(int i=0; i < SFS_MAX_OPEN_FILES; i++) {
                if(disk->open_list[i].used == 0) {
                        fp = i;
                        break;
                }
        }
        if(fp == -1) {
                printf("ERROR: max open file limit reached.\n");
                return -1;
        }
        /* Use file descriptor to access unused open file index its inode to fill in.*/
        file = &disk->open_list[fp];
        inode = &file->inode;
        /* Now we need to fill in the inode and open file struct data. */
        if(create_flag == 0) { // open an existing file...
                /* FIXME: Add support for opening existing files.
                 * Search the root directory data for a dir_entry with the same
                 * file name using the `sfs_find_dir_entry` function. That will
                 * tell us what inode number to read so we can fill in `inode`.
                 * If not found, throw error. */

                 //FIXME: Make it work when there are multiple directories in the system (filename will then be a path)

                printf("ERROR: SFS can't open existing files yet!\n");
                return -1;

        }
        else { // create a new file in the root directory...
                /* For new files, prepare a new inode for an empty file. */
                if(strlen(filename) >  SFS_NAME_LENGTH -1) {
                        printf("ERROR: file name %s is too long!\n", filename);
                        return -1;
                }
                // prepare inode in memory for the new file, then write to disk
                inode->type = 1; // type 1 = file
                inode->size = 0; // file is initially empty
                inode->used_blocks = 0;
                uint8_t inum = sfs_get_free_inode_index(disk);
                file->inode_index = inum;
                sfs_write_inode(disk, inum, inode);
                // prepare directory entry linked to this inode and write to disk
                struct sfs_dir_entry dir;
                dir.inum = inum;
                dir.strlen = strlen(filename);
                strcpy((char*)&dir.name, filename);

                /* Update the parent directory so it has a dir_entry for
                * the new file. Otherwise we won't be able to open it later! */
                struct sfs_inode* dir_inode = &disk->root_dir_inode;
                sfs_create_dir_entry(disk, dir_inode, &dir); // FIXME: make this function work!
        }
        // mark as used and set offset to start of file
        file->used = 1;
        file->cur_offset = 0;
        disk->open_files++;
        return fp;
}

/* Write nbytes of buf to an open file descriptor.
 * Return -1 on failure or the number of bytes successfully written.*/
int sfs_write(struct sfs_disk* disk, int filedes, void* buf, int nbytes)
{
        if(filedes < 0 || filedes >= SFS_MAX_OPEN_FILES || disk->open_list[filedes].used == 0) {
                printf("ERROR: tried to write to invalid file descriptor!\n");
                return -1;
        }
        struct sfs_open_file* file = &disk->open_list[filedes];
        struct sfs_inode* inode = &file->inode;
        if(inode->size + nbytes > SFS_BLOCK_SIZE) {
                printf("ERROR: SFS does not support files with more than 1 data block!\n");
                return -1;
        }
        // TODO: Currently only supports one block per file
        int block, offset_in_block;
        if(inode->used_blocks == 0) {
                block = sfs_get_free_block(disk);
                inode->used_blocks++;
                inode->block[0] = block;
        }
        else {
                block = inode->block[0];
        }
        offset_in_block = file->cur_offset;
        disk_write(disk->data, block, offset_in_block, buf, nbytes);
        file->cur_offset += nbytes;
        // Update file size in inode and write to disk
        // FIXME: file size doesn't work correctly if you seek back and then write
        inode->size += nbytes;
        sfs_write_inode(disk, file->inode_index, inode);
        return nbytes;
}

/* Read nbytes from an open file descriptor into buf.
 * Return -1 on failure or the number of bytes successfully read.*/
int sfs_read(struct sfs_disk* disk, int filedes, void* buf, int nbytes)
{
        if(filedes < 0 || filedes >= SFS_MAX_OPEN_FILES || disk->open_list[filedes].used == 0) {
                printf("ERROR: tried to read from invalid file descriptor!\n");
                return -1;
        }
        struct sfs_open_file* file = &disk->open_list[filedes];
        struct sfs_inode* inode = &file->inode;
        if(inode->size < nbytes) {
                printf("ERROR: SFS can't handle too long reads!\n");
                return -1;
        }
        int block, offset_in_block;
        block = inode->block[0]; // TODO: Currently only supports one block per file
        offset_in_block = file->cur_offset;
        disk_read(disk->data, block, offset_in_block, buf, nbytes);
        file->cur_offset += nbytes;
        return nbytes;
}

/* Change the read pointer in the file by `offset`.
 * If option is SEEK_SET, the offset is set to offset bytes.
 * If option is SEEK_CUR, the offset is set to its current location plus offset bytes.
 * If option is SEEK_END, the offset is set to the size of the file minus offset bytes.
 * Return 0 or -1 on failure.*/
int sfs_seek(struct sfs_disk* disk, int filedes, int offset, int option)
{
        if(filedes < 0 || filedes >= SFS_MAX_OPEN_FILES || disk->open_list[filedes].used == 0) {
                printf("ERROR: tried to seek in invalid file descriptor!\n");
                return -1;
        }
        printf("ERROR: seek is not yet supported!\n");
        return -1;
}

/* Given a pointer to the disk, and the name of a new directory, create that directory.
   Currently it will create directory in root level, unless ability to have multiple
   levels of directories in the file is added. Then dirname will give full path, 
   which must be parsed to find correct parent
*/
int sfs_mkdir(struct sfs_disk* disk, char* dirname){
        printf("ERROR: mkdir is not yet supported!\n");
        return -1;

        // TODO: Need to find a place for directory inode, and directory data

        // TODO: Need to write inode and data to disk
        // TODO: Need to update parent directory's inode

        // FIXME: Be able to add a directory somewhere other than in root
}

/* Given a poitner to the disk, removes the filename given. Filename contains directory structure
 * if the code supports more than 1 directory.
 * Returns 0 on success, -1 on failure. */
int sfs_rm(struct sfs_disk* disk, char* filename){
        //TODO: Need to implement once bitmaps are implemented correctly
        printf("ERROR: rm is not yet supported!\n");
        return -1;
}
