#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <inttypes.h>
#include <string.h>
#include <ctype.h>

#include "disk.h"
#include "sfs.h"
#include "filepath.h"


/* Given a pointer to a particular disk, read entry `n` from directory dir_inode, and store its info into `dir`. 
 * Returns 0 on success or -1 on failure */
int sfs_read_dir_entry(struct sfs_disk* disk, struct sfs_inode* dir_inode, int n, struct sfs_dir_entry* dir)
{

    int dir_entry_index = n;
    int index = ((SFS_DIR_ENTRY_SIZE * dir_entry_index )/ SFS_BLOCK_SIZE);
    int offset = 0;

    if ( dir_entry_index % 8 == 0 ){ //we have to correct offset (next block)
        offset = 0;
    } else { // offset calculation is valid (same block)
        offset = (dir_entry_index * SFS_DIR_ENTRY_SIZE);
    }

    int block = dir_inode->block[index];

    disk_read(disk->data,block,offset, &dir->inum, 1);
    disk_read(disk->data,block,offset+1, &dir->strlen, 1);
    disk_read(disk->data,block,offset+2, &dir->name, dir->strlen+1);

    if (dir->strlen != 0)
        return 0;

    return -1;
}

/* Create a new file entry in a directory based on info in `direntry`.
 * Returns index of the entry or -1 on failure */
int sfs_create_dir_entry(struct sfs_disk* disk, struct sfs_inode* parent_inode, struct sfs_dir_entry* child_dir_entry)
{

    int block_index;
    int offset;


    if (parent_inode->used_blocks == 0){
        //making sure that theres always at least 1 block to put dir entries in

        block_index = sfs_get_free_block(disk);
        parent_inode->used_blocks++;

        parent_inode->block[0] = block_index;
    }

    int parent_dir_entry_index = 0;
    int max_entries = ((parent_inode->used_blocks*SFS_BLOCK_SIZE)/SFS_DIR_ENTRY_SIZE); // so we don't read off bounds
    for (parent_dir_entry_index = 0; parent_dir_entry_index < max_entries; parent_dir_entry_index++){
        struct sfs_dir_entry tmp;
        int invalid = sfs_read_dir_entry(disk,parent_inode,parent_dir_entry_index,&tmp);
        if (invalid){
            printf("Using dir entry %d\n", parent_dir_entry_index);
            break; // can use parent_dir_entry_index
        }
    }

    if ( parent_dir_entry_index * SFS_DIR_ENTRY_SIZE >= parent_inode->used_blocks * SFS_BLOCK_SIZE ){ //index is off bounds
        //need to allocate a new block
        block_index = sfs_get_free_block(disk);
        parent_inode->block[parent_inode->used_blocks] = block_index;
        offset = 0;
        parent_inode->used_blocks++;
    } else { // the last block can fit another dir entry

        block_index = parent_inode->block[parent_inode->used_blocks-1];
        offset = (parent_dir_entry_index * SFS_DIR_ENTRY_SIZE);

    }

    //writing the child dir entry to disk
    disk_write(disk->data, block_index, offset, &child_dir_entry->inum, 1);
    disk_write(disk->data, block_index, offset+1, &child_dir_entry->strlen, 1);
    disk_write(disk->data, block_index, offset+2, &child_dir_entry->name, child_dir_entry->strlen+1);

    struct sfs_dir_entry parent_dir_entry;
    int invalid = sfs_read_dir_entry(disk, parent_inode, 0, &parent_dir_entry); //getting the parent dir entry (always in 0 pos)
    if (!invalid){
        sfs_write_inode(disk, parent_dir_entry.inum, parent_inode); //updating the parent dir entry with the new child
    } else {
        printf("ERROR: Invalid parent directory");
        return -1;
    }

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

    struct sfs_inode* parent = &disk->root_dir_inode;

    struct filepath_s filepath;
    filepath_init(&filepath, filename);

    filepath_traverse(&filepath);

    while (filepath_has_next(&filepath)){

        char* filename_base = filepath_next(&filepath);

        if (strcmp(filename_base, ".") == 0 && filepath_has_next(&filepath)) //because parent is already root inode, moving to next item in file path
            continue;

        int max_used_entries = parent->used_blocks * SFS_BLOCK_SIZE / SFS_DIR_ENTRY_SIZE;

        int i = 0;
        for(; i<max_used_entries; i++){
            struct sfs_dir_entry found_dir_entry;
            int invalid;
            invalid = sfs_read_dir_entry(disk, parent, i, &found_dir_entry);
            if (!invalid){
                if (strcmp(found_dir_entry.name, filename_base) == 0){ //we found the file we were looking for
                    entry->inum = found_dir_entry.inum;
                    entry->strlen = found_dir_entry.strlen;
                    strcpy(entry->name, found_dir_entry.name);

                    if (filepath_has_next(&filepath)){ // the current file is a dir
                        sfs_read_inode(disk, entry->inum, parent); // new parent set to current dir
                        break; //moving on to next item in the filepath
                    } else {
                        return 0; //found the right file
                    }
                }
            }
        }
    }

    return -1;
}
