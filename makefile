CC = gcc

all: oss worker

.SUFFIXES: .c .o

oss: oss.c header.h
        $(CC) -g -o oss oss.c -lpthread -lrt -lm

worker: worker.c header.h
        $(CC) -g -o worker worker.c -lpthread -lrt -lm

clean:
        $(RM) oss worker *.o
