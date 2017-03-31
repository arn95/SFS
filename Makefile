OBJS=test.o files.o directory.o inode.o superblock.o
CFLAGS=-g -I.  -std=c99
#-Wall -Wextra  # add these to cflags for verbose warnings
BIN=sfs
CC=gcc

%.o:%.c
	$(CC) $(CFLAGS) $(DEFINES) -o $@ -c $<

$(BIN): $(OBJS)
	$(CC) $(CFLAGS) $(DEFINES) -o $(BIN) $^

clean:
	rm $(BIN) $(OBJS)
