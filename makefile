CC=g++ -std=c++11
CFLAGS=-Wall -pedantic
OBJS=parser.o commands.o bitmap.o superblock.o inode.o cluster.o vfs.o
EXE=vfs

$(EXE): $(OBJS)
	$(CC) $(CFLAGS) $^ -o $@ -lm

.cpp.o:
	$(CC) $(CFLAGS) -c $^

