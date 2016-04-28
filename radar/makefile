CC = gcc
CFLAGS = -I.:/usr/include/libftdi1/
CFALGS += -pthread -lmpsse -lusb-1.0

default: collectData

collectData.o: collectData.c
	$(CC) -c collectData.c -o collectData.o

collectData: collectData.o
	$(CC) collectData.o -o collectData

clean:
	-rm -f *.o
	-rm -f collectData
