#ifndef SFS_H
#define SFS_H

#define SFS_NUM_BLOCKS 256      // total blocks on disk
#define SFS_BLOCK_SIZE 128      // size of each block in bytes
#define SFS_MAGIC 466           // unique number to identify file system type
#define SFS_BLOCKS_PER_INODE 28 // number of data blocks per inode
#define SFS_MAX_OPEN_FILES 8    // maximum files open at the same time
#define SFS_INODE_SIZE 32       // size of an inode in bytes
#define SFS_DATA_BLOCK_START 8  // block number for start of data region
#define SFS_INODE_BLOCK_START 3 // block number for start of inode
#define SFS_NAME_LENGTH 14      // maximum length of a file's name
#define SFS_DIR_ENTRY_SIZE 16   // size of a directory entry in bytes
#define SFS_INODE_BITMAP_START 1 // block number for start of inode bitmap
#define SFS_DATA_BITMAP_START 2  // block number for start of data bitmap

/*************** SFS ON-DISK DATA STRUCTS ***************/
/* These structs represent data stored on disk. */

/* Super block meta data about the disk. Stored in block 0. */
struct sfs_super {
        uint16_t magic;         // magic number to identify FS=466
        uint8_t inode_blocks;   // total blocks allocated for inodes
        uint8_t data_blocks;    // total blocks allocated for data
        uint8_t used_inodes;    // currently used inodes (each inode is smaller than a block)
        uint8_t used_data;      // currently used data blocks
};

/* inodes represent files or directories. An inode contains direct pointers
 * to the data blocks holding file contents or directory listings. The direct
 * pointers are the block index within the disk where the data is located. */
struct sfs_inode { // SFS_INODE_SIZE = 32 bytes per inode
        uint8_t type;           // 0=unused, 1=file, 2=dir
        uint16_t size;          // total data size in bytes
        uint8_t used_blocks;    // total used blocks
        uint8_t block[SFS_BLOCKS_PER_INODE]; // list of block indices
};

/* A dir_entry represents an entry in a directory (a file or nested directory)
 * A directory inode stores sfs_dir_entry's inside its data blocks. The
 * dir_entry has meta data about a file or directory inside its parent.
 * Unlike vsfs, we use a fixed size dir_entry.
 * Currently we only use dir_entries to represent files since we don't support
 * nested directories. */
struct sfs_dir_entry { // SFS_DIR_ENTRY_SIZE = 16 bytes per dir entry
        uint8_t inum;                   // inode index number for this file/dir
        uint8_t strlen;                 // length of the file name
        char name[SFS_NAME_LENGTH];     // null terminated file name string
};

/********************** SFS META DATA STRUCTS **********************/
/* These structs store meta data that is not written to disk. */

/* Struct representing an open file. Stored in memory in the sfs_disk below.*/
struct sfs_open_file {
        int used; // is this struct in use? 0=unused 1=used
        int cur_offset; // current offset for reading or writing in the file
        struct sfs_inode inode; // inode for file
        uint8_t inode_index; // inode number for file
};

/* This represents the overall disk and file system. Normally it would have
 * meta data about the file system as well as a device ID of where to access
 * the physical disk to store data.  Instead, we store data in a char* in
 * memory referenced by this struct. */
struct sfs_disk {
        char* data;                     // in-memory array representing the disk
        struct sfs_super super;         // super block of the disk
        struct sfs_open_file open_list[SFS_MAX_OPEN_FILES]; // array that stores info about open files
        int open_files;                 // number of files currently open
        struct sfs_inode root_dir_inode;// inode of the root directory so we can find files
};

// super block functions
int sfs_format(struct sfs_disk* disk);
int sfs_mount(struct sfs_disk* disk, char* dump_file_name);
int sfs_read_super(struct sfs_disk* disk);
int sfs_write_super(struct sfs_disk* disk, struct sfs_super* super);
void sfs_print_super(struct sfs_super* super);
uint8_t sfs_get_free_block(struct sfs_disk* disk);
uint8_t sfs_get_free_inode_index(struct sfs_disk* disk);
int sfs_dump(struct sfs_disk* disk, char* dump_file_name);

// inode functions
int sfs_read_inode(struct sfs_disk* disk, int index, struct sfs_inode* inode);
int sfs_write_inode(struct sfs_disk* disk, int index, struct sfs_inode* inode);
void sfs_print_inode(struct sfs_inode* inode);

// Directory functions
int sfs_create_dir_entry(struct sfs_disk* disk, struct sfs_inode* dir_inode,
        struct sfs_dir_entry* dir);
int sfs_read_dir_entry(struct sfs_disk* disk, struct sfs_inode* dir_inode,
        int n, struct sfs_dir_entry* dir);
void sfs_ls_dir(struct sfs_disk* disk, struct sfs_inode* dir_inode);
void sfs_print_dir_entry(struct sfs_disk* disk, struct sfs_dir_entry* dir);
int sfs_find_dir_entry(struct sfs_disk* disk, char* filename, struct sfs_dir_entry* entry);

// file operations
int sfs_open(struct sfs_disk* disk, char* filename, int create_flag);
int sfs_mkdir(struct sfs_disk* disk, char* dirname);
int sfs_read(struct sfs_disk* disk, int filedes, void* buf, int nbytes);
int sfs_write(struct sfs_disk* disk, int filedes, void* buf, int nbytes);
int sfs_close(struct sfs_disk* disk, int filedes);
int sfs_rm(struct sfs_disk* disk, char* filename);
int sfs_seek(struct sfs_disk* disk, int filedes, int offset, int option);

#endif
