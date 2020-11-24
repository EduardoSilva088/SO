CFLAGS = -Wall
CC = gcc

all:
	make argusd
	make argus

argus:argus.c 
	$(CC) $(CFLAGS) argus.c -o argus
argusd:argusd.c argus.h
	$(CC) $(CFLAGS) argusd.c -o argusd
clean:
	rm -rf *.o argus argusd fifoServer fifoClient *.txt