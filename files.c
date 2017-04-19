#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <inttypes.h>
#include <string.h>
#include <math.h>

#include "disk.h"
#include "sfs.h"

#include "filepath.h"

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

    struct sfs_open_file* file;
    struct sfs_inode* inode; // pointer to inode inside open_file struct

    /* Use file descriptor to access unused open file index its inode to fill in.*/
    file = &disk->open_list[fp];
    inode = &file->inode;
    /* Now we need to fill in the inode and open file struct data. */
    if(create_flag == 0) { // open an existing file...

        struct sfs_dir_entry found_dir_entry;
        int invalid = sfs_find_dir_entry(disk,filename,&found_dir_entry);
        if (!invalid){
            uint8_t inum = found_dir_entry.inum;
            sfs_read_inode(disk, inum, inode);
            file->used = 1;
            file->inode_index = inum;
            file->cur_offset = 0;
            return fp;
        } else {
            printf("ERROR: No dir_entry found for %s !\n", filename);
            return -1;
        }
    }
    else { // create a new file

        struct filepath_s filepath;
        filepath_init(&filepath, filename);

        filepath_traverse(&filepath); //initiated the curr_name and curr_index pointer

        struct sfs_inode* parent = &disk->root_dir_inode;

        while (filepath_has_next(&filepath)){

            char* filename_base = filepath_next(&filepath);

            if (strcmp(filename_base, ".") == 0) //parent is already root so move on
                continue;

            /* For new files, prepare a new inode for an empty file. */
            if(strlen(filename_base) >  SFS_NAME_LENGTH -1) {
                printf("ERROR: file name %s is too long!\n", filename_base);
                return -1;
            }

            // prepare inode in memory for the new file, then write to disk
            inode->type = (uint8_t) (filepath_has_next(&filepath) ? 2 : 1);
            inode->size = 0; // initially empty
            inode->used_blocks = 0;
            int inum = sfs_get_free_inode_index(disk);
            file->inode_index = inum;
            sfs_write_inode(disk, inum, inode);
            // prepare directory entry linked to this inode and write to disk
            struct sfs_dir_entry dir;
            dir.inum = inum;
            dir.strlen = (uint8_t) strlen(filename_base);
            strcpy((char*)&dir.name, filename_base);

            /* Update the parent directory so it has a dir_entry for
            * the new file. Otherwise we won't be able to open it later! */

            sfs_create_dir_entry(disk, parent, &dir);

            if (filepath_has_next(&filepath)){
                parent = inode; //the parent becomes the current inode we just made so we can add children to it
            } else { // last inode in the filepath being set
                file->used = 1;
                file->inode = *inode;
                file->inode_index = dir.inum;
                file->cur_offset = 0;
                disk->open_files++;
                return fp;
            }
        }

        return -1;
    }
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


    int bytes_to_fit = nbytes;
    int cur_offset;

    if (bytes_to_fit > inode->size)
        cur_offset = inode->size; //write at the end
    else
        cur_offset = 0; //write from beginning

    while (bytes_to_fit > 0){
        int last_block_free_space = (inode->used_blocks * SFS_BLOCK_SIZE) - inode->size;
        if (last_block_free_space >= bytes_to_fit){ //the last block can fit this size

            int block_index = inode->used_blocks-1; //last block index

            disk_write(disk->data, block_index, cur_offset, buf, last_block_free_space);

            //update counters
            bytes_to_fit -= last_block_free_space;
            inode->size += last_block_free_space;
            cur_offset = inode->size;

        } else {
            if (last_block_free_space > 0){ // last block cant fit the entire size user wants but can fit some

                int block_index = inode->used_blocks-1; //last block index

                disk_write(disk->data, block_index, cur_offset, buf, last_block_free_space);

                bytes_to_fit -= last_block_free_space;
                inode->size += last_block_free_space;
                cur_offset = inode->size;
            }

            int block_index = sfs_get_free_block(disk); //get new block index
            inode->block[inode->used_blocks] = block_index; // new block index is now in the next available block spot
            inode->used_blocks++;

            if (bytes_to_fit >= SFS_BLOCK_SIZE){ //write on the whole block
                disk_write(disk->data, block_index, cur_offset, buf, SFS_BLOCK_SIZE);
                bytes_to_fit -= SFS_BLOCK_SIZE;
                inode->size += SFS_BLOCK_SIZE;
                cur_offset = inode->size;
            } else { //write the remaining bytes
                disk_write(disk->data, block_index, cur_offset, buf, bytes_to_fit);
                inode->size += bytes_to_fit;
                bytes_to_fit = 0;
                cur_offset = inode->size;
            }

        }
    }

    file->cur_offset = cur_offset; //set offset to the file which should be the end of the file.

    sfs_write_inode(disk, file->inode_index, inode); //update inode
    return cur_offset; //return bytes written
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

    int bytes_to_read = nbytes;

    int cur_offset = file->cur_offset;
    int curr_block_index = 0;
    while (bytes_to_read > 0){
        int block = inode->block[curr_block_index++]; //start from block 0 and so on (memory is contiguous)
        if (bytes_to_read < SFS_BLOCK_SIZE){ //read only the requested amount of bytes from this current block
            disk_read(disk->data,block,cur_offset,buf,bytes_to_read);
            cur_offset += bytes_to_read;
            bytes_to_read = 0;
        } else { //have to read more than a block
            disk_read(disk->data,block,cur_offset,buf,SFS_BLOCK_SIZE);
            bytes_to_read -= SFS_BLOCK_SIZE;
            cur_offset += SFS_BLOCK_SIZE;
        }
    }

    file->cur_offset = cur_offset;

    return cur_offset; //this is how much we read
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

    struct sfs_open_file* open_file = &disk->open_list[filedes];

    switch(option){
        case SEEK_SET: {
            open_file->cur_offset = offset;
            return 0;
        } break;
        case SEEK_CUR: {
            open_file->cur_offset = (open_file->cur_offset + offset);
            return 0;
        } break;
        case SEEK_END: {
            open_file->cur_offset = (open_file->inode.size - offset);
            return 0;
        } break;
        default: {
            printf("ERROR: Invalid seek option!\n");
            return -1;
        };
    }
}

/* Given a pointer to the disk, and the name of a new directory, create that directory.
   Currently it will create directory in root level, unless ability to have multiple
   levels of directories in the file is added. Then dirname will give full path, 
   which must be parsed to find correct parent
*/
int sfs_mkdir(struct sfs_disk* disk, char* dirname){

    //root dir
    struct sfs_inode* parent = &disk->root_dir_inode;

    struct filepath_s filepath;
    filepath_init(&filepath, dirname);

    filepath_traverse(&filepath);

    while (filepath_has_next(&filepath)){

        char* filename = filepath_next(&filepath); //create all the dirs in this path

        if(strlen(filename) >  SFS_NAME_LENGTH -1) {
            printf("ERROR: file name %s is too long!\n", filename);
            return -1;
        }

        struct sfs_inode new_inode;

        new_inode.type = 2; //dir type
        new_inode.size = 0;
        new_inode.used_blocks = 0;

        int new_inode_index = sfs_get_free_inode_index(disk);

        sfs_write_inode(disk, new_inode_index, &new_inode);

        struct sfs_dir_entry new_inode_dir_entry;
        new_inode_dir_entry.inum = new_inode_index;
        new_inode_dir_entry.strlen = (uint8_t) strlen(filename);
        strcpy((char*)&new_inode_dir_entry.name,filename);

        sfs_create_dir_entry(disk,parent,&new_inode_dir_entry);

        parent->size += SFS_DIR_ENTRY_SIZE;

        if (filepath_has_next(&filepath))
            parent = &new_inode; //current created dir is the parent now so we can add children in the next iteration
    }

    return 1;
}

/* Given a poitner to the disk, removes the filename given. Filename contains directory structure
 * if the code supports more than 1 directory.
 * Returns 0 on success, -1 on failure. */
int sfs_rm(struct sfs_disk* disk, char* filename){
    //TODO: Need to implement once bitmaps are implemented correctly
    printf("ERROR: rm is not yet supported!\n");
    return -1;
}
