# Assignment4

This assignment is a simulation of a file system.

I abided the Loyola Honor code.o

# Student notes
## Working
The parts that have been added and work correctly:

All the required code is implemented.

Implemented:
A. Dirs able to hold more than 1 file.
B. Inode and Data block distribution is done entirely through bitmaps.
D. Files able to use more than 1 block of data (dir inodes and file inodes)
F. Seeking to different parts of the file

Note: I added bitmap.c/.h and filepath.c/.h

## Not Working
The parts that have been added and don't quite work correctly, with information on what seems to be wrong with them:

Path resolving is not entirely functional. If you want to create a file given a path it wont work correctly.
The root inode being able to hold a dir entry of itself is weird and I tried to fix it but it meant changing how this assignment is organized.
I am not sure what the issue is exactly. I kept getting segfaults and I spent a lot of time debugging.
It could be that the inodes are not being chained correctly.
Finding files in nested directories works totally fine as well as actually creating them. When it comes to accessing them its weird.


# ----- Ellie and Dan’s Notes on the SFS ------
In these notes you will find the following: an overall description of the SFS system, a description of the code and how it is organized, descriptions of each of the required changes, and descriptions for each of the additional changes.

Code is compiled with "make" and run with "./sfs"

## DESCRIPTION OF THE SYSTEM
The Serenity File System is currently written as a simulation of a file system. Instead of directly interacting with a real file system, the code interacts with a char* array that acts as the disk.  Essentially the way it works is that there are functions for interacting with the disk in the same way that code would interact with a real disk – that is the file system code. In general, most work of the file system will be done in memory, i.e. in variables. When it is time to write to the disk, then the changes will be saved to the char* array disk. There is no saving to the actual disk of the machine.

This system has no caching or buffering. As soon as all changes to a block are determined, that block is written to disk (i.e. saved in the char* array). When data is read in, it is by block. In this case, reading in data from disk means copying it over from the char* array to a new array.

*Setup*: Block 0 is the superblock, block 1 is the inode bitmap, block 2 is the data bitmap, blocks 3-7 are for inodes, and blocks 8-255 are for data. Blocks are of size 128 bytes.

*Details:* The Serenity File System is based on the Very Simple File System, with some note-able exceptions. Below we describe those differences only. If you need reminders on the VSFS, see OSTEP chapter 40, or your OS notes.

*Bitmaps:* Although there is space allocated in the disk for bitmaps, the current SFS does not use them. Instead, since nothing is ever removed, the superblock has a counter for how many blocks are used for inodes and datablocks, which then denotes the next available block.

*Block choice:* The first available block is always the one assigned when a block is needed. There is no freelist.

*Directory structure:* Each directory entry is slightly simplified from the VSFS. Instead of having a record length, each record is of fixed length. There is still a length for the name, of course, since most names will not take up the entire record length. The rest is the same as VSFS.

*Directories:* Currently the file system only supports 1 directory, the root directory. The root directory’s inumber is stored in the sfs_disk struct.

*Filename:* Filenames are limited in length to 14 characters total.

*File length:* Currently files are limited to using 1 block of space, although the inode structure does have space for direct pointers to more than one block.

*Inodes:* Inodes keep track of the following: file type (file or directory only), size of file in bytes, the number of used blocks, and an array of direct pointers to blocks. In this system, the direct pointer is an integer hold the block's number.

## DESCRIPTION OF THE CODE
There are no known bugs in the provided code, other than the fact that the listed functionality is missing. The code is organized into a few different sets of structs and functions. There are FIXME comments in areas where code should be added first to get a function working, with TODO comments in areas that should be worked on after the FIXME areas. 

Throughout the code a pointer to the disk is passed among functions instead of being a global variable. Although that is the case, the main function that acts as the user program should never directly interact with the disk, but only call the functions in files.c or directory.c. Each .c file represents a set of related functions:

Sfs.h:
* Includes all global parameters, structs, and function headers
* The first set of structs represent the data that is stored to disk
* The second set of structs are purely for in-memory functions, and are never saved to disk
* Includes a list of all function headers, organized by the C file to which they belong
* You should NOT edit any structs

Disk.h:
* You should NOT edit this file. 
* Includes the functions for directly interacting with the disk
* You must use these functions when interacting with the disk array

Superblock.c:
* Functions for initializing or interacting with the superblock
* Note that format and mount exist here. For a real file system they would be separate, but we need them to initialize each time we run the program since we are a simulation.

Inode.c
* Functions for interacting with an inode
* Note that this includes a print function that may be handy for debugging

Directory.c
* Functions for interacting with a directory
* Note that this includes a print function that may be handy for debugging, although it won’t work correctly until you fully implement directories.
* The only functions here that would be called directly by a program using your file system are mkdir and rm.

Files.c
* Functions for interacting with files. 
* These are the main functions that a program would call to interact with your file system. 

Test.c
* Main function is in this file
* Use the provided test functions, and other testing functions you write in this file, to test your code. We have provided for you some simple tests to see the current state of the file system.


## DESCRIPTION OF NECESSARY CHANGES

**1 - Opening an existing file**

The sfs_open function has correct code for creating a new file in the root directory, but is missing the code to open a pre-existing file. You need to add in the code for opening an existing file by searching through the root directory’s entries to find a file with a matching name. You also need to fix the read directory entry function so that both cases of open work. You will need to edit or complete these functions:

``` int sfs_open(struct sfs_disk* disk, char* filename, int create_flag); ```

``` int sfs_find_dir_entry(struct sfs_disk* disk, char* filename, struct sfs_dir_entry* entry); ```

``` int sfs_read_dir_entry(struct sfs_disk* disk, struct sfs_inode* dir_inode, int n, struct sfs_dir_entry* dir); ```


**2 - Allowing a directory to hold more than 1 file in it. This is already partially implemented.**

Currently you can open multiple new files but the root directory is never updated with information about these files. The test code opens those files, but the file system code never saves them to the root directory inode, so you can never find them again later. You need to update the code so new files are added to the directory structure and saved on disk as an sfs_dir_entry. You will need to update the following function if you didn't write it to work with this situation when fixing the file opening problem:

``` int sfs_read_dir_entry(struct sfs_disk* disk, struct sfs_inode* dir_inode, int n, struct sfs_dir_entry* dir); ```



## DESCRIPTION OF ADDITIONAL CHANGES
For many of these changes you may wish to provide additional test cases that demonstrate that they are working correctly.

**A - Being able to have more than 1 directory**

Currently the root directory is the only directory, but that’s pretty lame. For this change you will need to modify the SFS so that additional directories can be added. Note that this involves being able to add a new directory, as well as being able to find files that are in nested directories (i.e.  ```/foo/olsen/blah/bar.txt```). You will have to edit the following functions:

``` int sfs_read_dir_entry(struct sfs_disk* disk, struct sfs_inode* dir_inode, int n, struct sfs_dir_entry* dir); ```

``` int sfs_mkdir(struct sfs_disk* disk, char* dirname); ```

``` int sfs_open(struct sfs_disk* disk, char* filename, int create_flag); ```

``` int sfs_find_dir_entry(struct sfs_disk* disk, char* filename, struct sfs_dir_entry* entry); ```

**B - Converting the file system to use a bitmap to track used inodes and datablocks**

Without a bitmap, it is impossible to deal with deleted files, as how will you know where the holes exist? Currently the system just knows how many blocks are used, assumes they are in order from the start, and gives out the next available one. For this change you will need to use the bitmap blocks in the SFS to hold bitmaps for the inodes and data blocks. To implement this you will have to practice your bit operations—a block in SFS is 128 bytes, but there are a total of 256 blocks, so you can’t use an entire byte to represent whether a block is used or not. Remember to update your bitmap whenever a new inode or data block is allocated. You will need to modify the following functions (you can ignore functions that aren't already implemented and that you aren't implementing for the assignment):

```int sfs_open(struct sfs_disk* disk, char* filename, int create_flag);```

```int sfs_create_dir_entry(struct sfs_disk* disk, struct sfs_inode* dir_inode, struct sfs_dir_entry* dir);```

``` int sfs_write(struct sfs_disk* disk, int filedes, void* buf, int nbytes) ``` (only if implement multiple blocks)

``` uint8_t sfs_get_free_block(struct sfs_disk* disk); ```

``` uint8_t sfs_get_free_inode_index(struct sfs_disk* disk) ```


**C - Being able to remove files. Can only be implemented if bitmaps is already implemented**

You first need to get bitmaps working before you can allow removal of files. To implement removal of files you will need to remove all references to the file, and treat its inode and data nodes as available. Your code should be able to remove a file given the file name (and path, if multiple directories have been implemented). Depending on how generically you implemented your other changes, you may need to modify how some of your other code works so that it still functions after this change. Code should go in the function:

```int sfs_rm(struct sfs_disk* disk, int filedes);```

**D - Allowing files to use more than 1 block**

Currently a file’s inode has direct pointers to blocks, but the code only supports a file using 1 block. You need to update the code so that a file can span multiple blocks. You should continue to use the policy of assigning the first available block to the file.  You will need to implement this change in functions that both read and write files, as each may need to interact with all blocks of a file. Note that although in practice many of your blocks will probably be contiguous, you may NOT assume in your code that each block in the file is in order on the disk – use the direct pointers instead. In this code the direct pointers are just indices of the blocks. If you implement bitmaps, you need to also ensure the bitmaps are properly updated if you need new blocks on a write. You will need to modify:

``` int sfs_write(struct sfs_disk* disk, int filedes, void* buf, int nbytes) ```

``` int sfs_read(struct sfs_disk* disk, int filedes, void* buf, int nbytes) ```

**E - Saving a copy of the disk’s contents to a real file on your computer and be able to load it when the program starts**

Since you are only simulating a disk, it may be useful to save the contents of it to your real hard drive so that your program can open it again later and start from the same point. For this option you must implement both sides: saving the disk to a file on the real disk, and loading a file of that format into the file system at the start. For loading the file system for the start you must modify the mount function so that it reads from the given filename. Currently it has code for starting up your file system from scratch:

```int sfs_mount(struct sfs_disk* disk, char* dump_file_name);```

To save the current state of your simulated disk to a real file on your computer, edit the function:

```int sfs_dump(struct sfs_disk* disk, char* dump_file```

**F - Being able to seek to a specific location in a file (such as with lseek)**

A program interacting with your file system may wish to move to a different location in a file than the current offset. A seek function will provide that functionality. Your seek function must be able to deal with 3 options similar to what lseek deals with: move a certain distance from the current offset, move a certain distance after the start of the file, and move a certain distance before the end of the file (this last one is different from lseek). Your function should return an error value and not change the offset if moving to the specified location is outside the bounds of the file. Your code should be in the function:

``` int sfs_seek(struct sfs_disk* disk, int filedes, int offset, int option); ```
