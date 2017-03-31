

### `sfs_format()` - Format the disk
Formatting the disk erases all of its contents and initializes the super block and related meta data.
 - Zero the memory used to represent the disk
 - Setup the superblock meta data (`struct sfs_super`)
 - Setup the root inode (`struct sfs_inode`). Currently this is not used for anything.
 - Write both the superblock and root inode to the disk

### `sfs_open()` - Open a new file
If this is a new file:
 - Find a free file descriptor. Mark it as used and set the file offset to the start of the file.
 - Prepare an inode for the new file. The file is empty at this point, so don't allocate any data blocks yet.
 - Write the new inode to disk
 - Prepare a directory entry for this file containing the inode index and the file name.
 - Write the new directory entry to the root inode's data block using `sfs_create_dir_entry`.

If this is an existing file:
 - *FIXME:* not supported yet!

return the file pointer--i.e., the index into the open files array--to the calling function on success, or -1 on failure.

### `sfs_write()` - Write to an open file
This function is used to write data to a file that has previously been opened with `sfs_open()`. Writing to an unused file pointer should return -1, otherwise the number of bytes written is returned. This might be less than the requested write size if the disk runs out of space or the inode can't hold another data block pointer.
 - Verify there is enough space for the new data
 - *FIXME:* Current code only supports files with at most one data block
 - If there aren't enough data blocks allocated yet to store the write, find a free data block and add it to the list of blocks for the inode. (Currently supports a max of 1 data block per inode)
 - Determine which block, and which offset within the block to write to based on the current offset in the file descriptor struct.
 - Write the data to the appropriate block and offset
 - Increment the size of the file in the inode and the current offset in the file descriptor.
 - Write the updated inode to disk.

### `sfs_read()` - Read from an open file
This function attempts to read `nbytes` from an open file. It returns -1 on error, or the number of bytes read (at most nbytes).
 - Find the file descriptor struct and inode
 - Use the current file offset to determine which block and block offset to read from.
 - *FIXME:* Currently the code only supports files with at most one data block!
 - *FIXME:* What happens if I try to read more data than fits in a block?
 - Read from the disk into buf
 - Adjust the offset in the file descriptor struct
 - Return the number of bytes read.
**Could have students implement this function after giving them the write version--it is almost a copy/paste and would be easy to test.**

## Multiple File Support
To support multiple files you need to a directory so the file system can find them by name. For simplicity we begin with support for a single root directory containing files (but no nested directories). The root directory inode is always the inode with index 0, and it uses the first data block to store directory entries.

**Deleting files is pretty tricky. The dir inode will have blocks that are fragmented. When you are looking for a file, do you always have to scan all the data blocks of the file?**

### `sfs_read_dir_entry()` - Read an entry from a directory
This function will read one entry (i.e., file or nested directory if supported) from a directory data block.  **Note:** in the textbook they suggest using an inode number of 0 in a directory entry to indicate that the entry is empty, but we can't do that here since we use inode 0 for the root inode. Instead, to determine if an inode is in use we must check the length of the name. If it has zero length, the entry is considered empty.
 - Find the entry's index in the correct data block.
 - *FIXME:* The current code only supports one data block worth of entries!
 - Read the data from disk and store it into the `sfs_dir_entry` struct.
 - Return an error if the entry is not used or parameters are invalid.

### `sfs_create_dir_entry()` - Create a new entry in a directory
This function will create a new entry (i.e., file or nested directory if supported) in the directory's data block. The `struct sfs_dir_entry` provided to the function must already have the file name, name length, and inode stored inside of it.  This should have been done by something like `sfs_open()`.
 - Find the next free entry in the inode's data blocks by searching for an invalid (empty) entry.
 - FIXME: Currently only supports one data block worth of entries!
 - Write the inode, name, and length to disk.

