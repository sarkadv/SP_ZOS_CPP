CC=g++ -std=c++11
CFLAGS=-Wall -pedantic
OBJS=parser.o fs.o
EXE=fs

$(EXE): $(OBJS)
	$(CC) $(CFLAGS) $^ -o $@ -lm

.cpp.o:
	$(CC) $(CFLAGS) -c $^

