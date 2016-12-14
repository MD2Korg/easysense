CC = gcc
CFLAGS = -I.:/usr/include/libftdi1/
CFLAGS += -pthread -lmpsse -lusb-1.0

default: collectData_GPIO

collectData_GPIO.o: collectData_GPIO.c
	$(CC) $(CFLAGS) -c collectData_GPIO.c -o collectData_GPIO.o

collectData_GPIO: collectData_GPIO.o
	$(CC) $(CFLAGS) collectData_GPIO.o -o collectData_GPIO

clean:
	-rm -f *.o
	-rm -f collectData_GPIO
