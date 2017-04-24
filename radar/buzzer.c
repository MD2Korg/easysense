//gcc -pthread -I /home/osu/include/libftdi1/ -I /home/osu/include  -L /home/osu/lib64 -L /home/osu/lib collectData.c  -lmpsse -lusb-1.0 -lftdi1 -o collectData.o
// gcc -pthread -I /usr/include/libftdi1/ collectData.c  -lmpsse -lusb-1.0 -o collectData.o

#include <stdio.h>
#include <stdlib.h>
#include <mpsse.h>
#include <time.h>
#include <string.h>
#include <sys/resource.h>
#include <sys/timerfd.h>
#include <unistd.h>
#include <sys/epoll.h>
#include <errno.h>
#include <inttypes.h>
#include "NoveldaRadar.h"  //Contains all the functions and global variables
#include <pthread.h>

#define READM 0x01	/*Address is Or'ed with this in case of read operation*/
#define WRITEM 0x00	/*Address is Or'ed with this in case of write operation*/
#define SLAVEADD 0xD0
#define SLAVEADD1 0x46
#define COMMANDMODE 0x30 /*Address pointer is Or'ed to use in command mode*/


void createport(unsigned char portNum)
{
char command[400];
(sprintf(command,"echo %d > /sys/class/gpio/export",portNum )  );
system(command);
}

void setupPort(unsigned char portNum,unsigned char direction  )
{
char command[400];
if(direction == 1)
{
(sprintf(command, "echo high > /sys/class/gpio/gpio%d/direction",portNum));
system(command);
}
else
{
(sprintf(command,"echo low > /sys/class/gpio/gpio%d/direction",portNum ));
system(command);
}
}

void setValue(unsigned char portNum, unsigned char value )
{
char command[400];
(sprintf(command, "echo %d > /sys/class/gpio/gpio%d/value",value,portNum ) );
system(command);
}


int main(int argc, char **argv)
{
	unsigned char gpio_devicePower = 48;
	unsigned char gpio_RF_vcc = 13;
	unsigned char gpio_analog_vcc = 49;
	struct mpsse_context *flash=NULL;
	createport(gpio_devicePower );
	createport(gpio_RF_vcc);
	createport(gpio_analog_vcc);

        setupPort(gpio_devicePower,1);
	setupPort(gpio_RF_vcc,1);
	setupPort(gpio_analog_vcc,1);

	setValue(gpio_devicePower,1);
	setValue(gpio_RF_vcc,1);
	setValue(gpio_analog_vcc,1);
	sleep(1);
	flash = Open(0x0403,0x6010,GPIO,TWELVE_MHZ,MSB,IFACE_B,NULL,NULL,1 );
	PinHigh(flash,GPIOH0);
	PinHigh(flash,GPIOH0);
	sleep(5);
	PinLow(flash,GPIOH0);
	Close(flash);

	setValue(gpio_RF_vcc,0 );
	setValue(gpio_analog_vcc,0);
	setValue(gpio_devicePower,0);

	return 0; //Return success
}
