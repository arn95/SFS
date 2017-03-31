#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <inttypes.h>
#include <string.h>

#include "disk.h"
#include "sfs.h"

/* Read inode at the specified index into the inode struct */
int sfs_read_inode(struct sfs_disk* disk, int index, struct sfs_inode* inode)
{
        int block, offset;
        block = SFS_INODE_BLOCK_START + index * SFS_INODE_SIZE / SFS_BLOCK_SIZE;
        offset = (index * SFS_INODE_SIZE) % SFS_BLOCK_SIZE;
        disk_read(disk->data, block, offset, &inode->type, 1);
        disk_read(disk->data, block, offset+1, &inode->size, 2);
        disk_read(disk->data, block, offset+3, &inode->used_blocks, 1);
        disk_read(disk->data, block, offset+4, &inode->block, SFS_BLOCKS_PER_INODE);
}

/* Write inode at the specified index from the inode struct */
int sfs_write_inode(struct sfs_disk* disk, int index, struct sfs_inode* inode)
{
        int block, offset;
        printf("LOG: Writing inode %d\n", index);
        block = SFS_INODE_BLOCK_START + index * SFS_INODE_SIZE / SFS_BLOCK_SIZE;
        offset = (index * SFS_INODE_SIZE) % SFS_BLOCK_SIZE;
        disk_write(disk->data, block, offset, &inode->type, 1);
        disk_write(disk->data, block, offset+1, &inode->size, 2);
        disk_write(disk->data, block, offset+3, &inode->used_blocks, 1);
        disk_write(disk->data, block, offset+4, &inode->block, SFS_BLOCKS_PER_INODE);
}

/* Print out an inode's type, size, and list of data blocks. */
void sfs_print_inode(struct sfs_inode* inode)
{
        char type;
        switch(inode->type) {
                case 0:
                        type = 'U'; // unused
                        break;
                case 1:
                        type = 'F'; // file
                        break;
                case 2:
                        type = 'D'; // directory
                        break;
                default:
                        printf("\n");
                        printf("ERROR: Invalid inode type\n");
        }
        printf("    %c  %6d bytes  %6d blocks: ", type, inode->size, inode->used_blocks);
        for(int i=0; i < inode->used_blocks; i++) {
                printf("%d ", inode->block[i]);
        }
        printf("\n");
}
