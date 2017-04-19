#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <inttypes.h>
#include <string.h>

#include "disk.h"
#include "sfs.h"

#include "bitmap.h"


/* Clear "disk" then initialize the magic number, inode,
 * and block counts in a new super block */
int sfs_format(struct sfs_disk* disk)
{
    struct sfs_super super;

    memset(disk->data, 0, 1);
    super.magic = SFS_MAGIC;
    /* Disk structure:
     * [SFFIIIIID...D] S=super, F=free map, I=inode block, D=data block
     * [012345678...255] */
    super.inode_blocks = SFS_DATA_BLOCK_START - SFS_INODE_BLOCK_START;
    super.data_blocks = SFS_NUM_BLOCKS - SFS_DATA_BLOCK_START;
    super.used_inodes = 1;
    super.used_data = 1;

    sfs_write_super(disk, &super);

    //root dir inode
    struct sfs_inode root_inode;
    root_inode.type = 2; // directory inode
    root_inode.size = 16; //will contain a dir_entry of itself
    root_inode.block[0] = sfs_get_free_block(disk); //reserve data block
    root_inode.used_blocks = 1;

    int root_inode_index = sfs_get_free_inode_index(disk);
    sfs_write_inode(disk, root_inode_index, &root_inode);

    struct sfs_dir_entry root_dir_entry;
    root_dir_entry.inum = 0;
    strcpy((char*)&root_dir_entry.name, "./");
    root_dir_entry.strlen = 2;

    int block_index = root_inode.block[0];
    int offset = 0;

    //writing root dir entry on root inode
    disk_write(disk->data, block_index, offset, &root_dir_entry.inum, 1);
    disk_write(disk->data, block_index, offset+1, &root_dir_entry.strlen, 1);
    disk_write(disk->data, block_index, offset+2, &root_dir_entry.name, root_dir_entry.strlen+1);

    return 0;
}

/* Read in the super block and setup sfs meta data.
 * dump_file_name is the path to a disk dump created by the sfs_dump() function,
 * or it can be NULL to mount a fresh disk. */
int sfs_mount(struct sfs_disk* disk, char* dump_file_name)
{
    if(dump_file_name != NULL) {
        // TODO: read dump_file_name into disk->data and then read the super (optional)
    }
    /* Read the super block, root inode, and clear open file data structure */
    sfs_read_super(disk);
    sfs_read_inode(disk, 0, &disk->root_dir_inode);
    disk->open_files=0;
    for(int i=0; i < SFS_MAX_OPEN_FILES; i++) {
        disk->open_list[i].used = 0;
    }
    return 0;
}

int sfs_read_super(struct sfs_disk* disk)
{
    struct sfs_super* super = &disk->super;
    disk_read(disk->data, 0, 0, &super->magic, 2);
    if(super->magic != SFS_MAGIC) {
        printf("Super block has invalid magic number! %d\n", super->magic);
        return -1;
    }
    disk_read(disk->data, 0, 2, &super->inode_blocks, 1);
    disk_read(disk->data, 0, 3, &super->data_blocks, 1);
    disk_read(disk->data, 0, 4, &super->used_inodes, 1);
    disk_read(disk->data, 0, 5, &super->used_data, 1);

    return 0;
}

int sfs_write_super(struct sfs_disk* disk, struct sfs_super* super)
{
    disk_write(disk->data, 0, 0, &super->magic, 2);
    disk_write(disk->data, 0, 2, &super->inode_blocks, 1);
    disk_write(disk->data, 0, 3, &super->data_blocks, 1);
    disk_write(disk->data, 0, 4, &super->used_inodes, 1);
    disk_write(disk->data, 0, 5, &super->used_data, 1);
    return 0;
}

void sfs_print_super(struct sfs_super* super)
{
    printf("Super block info \n");
    printf("  Magic number: %"PRIu16"\n", super->magic);
    printf("  Inode blocks: %"PRIu8"  count: %"PRIu8"\n",
           super->inode_blocks, super->inode_blocks * SFS_BLOCK_SIZE / SFS_INODE_SIZE);
    printf("  Data blocks:  %"PRIu8"\n", super->data_blocks);
    printf("  Inodes used:  %"PRIu8"\n", super->used_inodes);
    printf("  Data used:    %"PRIu8"\n", super->used_data);
}

/* Return the next free data block, or 0 on error. */
int sfs_get_free_block(struct sfs_disk* disk)
{

    struct bitmap_t bitmap;
    disk_read(disk->data,SFS_DATA_BITMAP_START,0,&bitmap,SFS_BLOCK_SIZE);
    int bit;
    for(bit = 0; bit<MAX_BITS; bit++){

        if (bit % 8 == 0){ //if this is the 8th,16th,32nd bit etc.
            unsigned char c = bitmap.array[GET_CHAR_INDEX(bit)];
            if (c == 0xFF){ //move fast through the bitmap (1111 1111) <- all taken
                bit += 7;
                continue;
            }
        }

        if (bitmap_check(&bitmap,bit) == 0){
            // bit represents the address of a free block.
            disk->super.used_data++;
            bitmap_set(&bitmap,1,bit);
            disk_write(disk->data,SFS_DATA_BITMAP_START,0,&bitmap,SFS_BLOCK_SIZE);
            int address = SFS_DATA_BLOCK_START + bit * SFS_BLOCK_SIZE / SFS_BLOCK_SIZE;
            return address;
        }

    }

    printf("ERROR: Out of data blocks\n");
    return 0;
}


/* Return the next free inode index, or 0 on error. */
int sfs_get_free_inode_index(struct sfs_disk* disk)
{

    struct bitmap_t bitmap;
    disk_read(disk->data,SFS_INODE_BITMAP_START,0,&bitmap,SFS_BLOCK_SIZE);
    int bit;
    for(bit = 0; bit<MAX_BITS; bit++){

        if (bit % 8 == 0){ //if this is the 8th,16th,32nd bit etc.
            unsigned char c = bitmap.array[GET_CHAR_INDEX(bit)];
            if (c == 0xFF){ //move fast through the bitmap if all of the spots are taken
                bit += 7;
                continue;
            }
        }

        if (bitmap_check(&bitmap,bit) == 0){
            // bit represents the index of a free inode.
            disk->super.used_inodes++;
            bitmap_set(&bitmap,1,bit);
            disk_write(disk->data,SFS_INODE_BITMAP_START,0,&bitmap,SFS_BLOCK_SIZE); //update bitmap on disk
            return bit;
        }

    }

    printf("ERROR: Out of inode blocks\n");
    return 0;
}

/* Dump the contents of the file system to a file on disk. Return 0 on succes,
 * or -1 on failure.*/
int sfs_dump(struct sfs_disk* disk, char* dump_file_name) {
    // TODO: dump disk->data to a file so it can be reloaded and mounted later
    return -1;
}
