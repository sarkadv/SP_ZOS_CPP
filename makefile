CC=g++ -std=c++11
CFLAGS=-Wall -pedantic
OBJS=vfs.o parser.o commands.o bitmap.o superblock.o data_block.o inode.o directory.o
EXE=vfs

$(EXE): $(OBJS)
	$(CC) $(CFLAGS) $^ -o $@ -lm

.cpp.o:
	$(CC) $(CFLAGS) -c $^


