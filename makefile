CC=g++ -std=c++11
CFLAGS=-Wall -pedantic
OBJS=parser.o commands.o fs.o
EXE=vfs

$(EXE): $(OBJS)
	$(CC) $(CFLAGS) $^ -o $@ -lm

.cpp.o:
	$(CC) $(CFLAGS) -c $^

