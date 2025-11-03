CC=gcc
CFLAGS=-I. -pthread

all: parallel_min_max process_memory parallel_sum

parallel_min_max: parallel_min_max.o utils.o find_min_max.o
	$(CC) -o parallel_min_max parallel_min_max.o utils.o find_min_max.o $(CFLAGS)

parallel_min_max.o: parallel_min_max.c utils.h find_min_max.h
	$(CC) -c parallel_min_max.c $(CFLAGS)

process_memory: process_memory.o
	$(CC) -o process_memory process_memory.o $(CFLAGS)

process_memory.o: process_memory.c
	$(CC) -c process_memory.c $(CFLAGS)

utils.o: utils.c utils.h
	$(CC) -c utils.c $(CFLAGS)

find_min_max.o: find_min_max.c find_min_max.h
	$(CC) -c find_min_max.c $(CFLAGS)

parallel_sum: parallel_sum.o utils.o sum.o
	$(CC) -o parallel_sum parallel_sum.o utils.o sum.o $(CFLAGS)

parallel_sum.o: parallel_sum.c utils.h sum.h
	$(CC) -c parallel_sum.c $(CFLAGS)

sum.o: sum.c sum.h
	$(CC) -c sum.c $(CFLAGS)

clean:
	rm -f *.o process_memory parallel_min_max parallel_sum

.PHONY: all clean