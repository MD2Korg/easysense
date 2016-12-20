gcc -pthread -L /usr/local/lib -L /usr/lib -I /usr/include/ -I /usr/include/libftdi1/ -lusb-1.0 -lmpsse collectData_GPIO.c -o collectData_GPIO.o
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/usr/local/lib:/usr/lib
./collectData_GPIO.o

