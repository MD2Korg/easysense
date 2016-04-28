/*Compilation instruction*/
/* gcc -I /home/osu/include/libftdi1/ -I /home/osu/include  -L /home/osu/lib64 -L /home/osu/lib ChipIdMod.c  -lmpsse -lusb-1.0 -lftdi1 -o chipIdMod.o*/

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

#define READM 0x01	/*Address is Or'ed with this in case of read operation*/
#define WRITEM 0x00	/*Address is Or'ed with this in case of write operation*/
#define SLAVEADD 0xD0
#define SLAVEADD1 0x42
#define COMMANDMODE 0x30 /*Address pointer is Or'ed to use in command mode*/



int main(void)
{
	// Sets the priority of the current process to the highest level.
	//setpriority(PRIO_PROCESS, 0, -20);	
	//Timer related variables
	timerEventTrigger *et1 = (timerEventTrigger*)malloc(sizeof(timerEventTrigger))  ;
	struct itimerspec oldRepeatPeriod;
	struct epoll_event ev;
	struct timeval t1,t2,t3,t4,t5,t6;
	unsigned char flagRandomOrder;
	//File handling related variables
	char fileNameConfig[300];
	long int counterFrames=0;
	double fps=0.0,sweepTime=0.0;
	FILE *fp = NULL,*fp1=NULL,*fp2;
	char id[80];

		
	char fileName[300],fileNameFinal[310],fileNameFinalSwitch[316],fileNameFPS[316];	
	
	int selectionOrder[4]={0,5,10,15};

	unsigned char readbits;
	unsigned char readbits1[24];
	char command[10] ={0};
	unsigned char dataRead[3000]={0};	
	//MPSSE related variables
	struct mpsse_context *flash = NULL,*flash1 = NULL;
	int latencyTimer = 1;
	int i=0;
    
	int counterRadarSelect =0;
	fflush(stdin);
	printf("before open\n");

  //flash = Open(0x0403, 0x6010, SPI0, ONE_MHZ, MSB, IFACE_B, NULL, NULL,latencyTimer);
 //flash1  = Open(0x0403, 0x6010, I2C, ONE_HUNDRED_KHZ, MSB, IFACE_A, NULL, NULL,latencyTimer);

//(flash1 = MPSSE(I2C,ONE_HUNDRED_KHZ, MSB,latencyTimer));
//  flash = OpenIndex(0x0403, 0x6010, SPI0, ONE_MHZ, MSB, IFACE_A, NULL, NULL, 0,latencyTimer);
if((flash = MPSSE(SPI0,TWELVE_MHZ,MSB,latencyTimer))!=NULL  && flash->open)
{
		//Radar ChipID read
		command[0] = ChipID | READ;
		command[1] = ChipIDLength;	
		Start(flash);
		FastWrite(flash, command, 2);
		FastRead(flash, readbits1,2);
		Stop(flash);	
		printf("CHip Id from radar is %x\t%x\n",readbits1[0],readbits1[1]);

		//MOtion Sensor CHIPID read
	/*command[0] = SLAVEADD | WRITEM; // Powering Motion Sensor ON
		Start(flash1);
		Write(flash1,command,1);
		if(GetAck(flash1) == ACK)
			{
			command[0] = 0x6B;
			Write(flash1,command,1);
			command[0] = 0x00;
			Write(flash1,command,1);
			}
		Stop(flash1);
		
		command[0] = SLAVEADD | WRITEM; // Choosing I2C interface
		Start(flash1);
		Write(flash1,command,1);
		if(GetAck(flash1) == ACK)
			{
			command[0] = 0x6A;
			Write(flash,command,1);
			command[0] = 0x00;
			Write(flash1,command,1);
			}
		Stop(flash1);
		
		command[0] = SLAVEADD | WRITEM; // Reading chip ID
		Start(flash1);
		Write(flash1,command,1);
		if(GetAck(flash1) == ACK)
			{
			command[0] = 0x75;
			
			Write(flash1,command,1);
			if(GetAck(flash1) == ACK)
				{
				Start(flash1);
				command[0] = SLAVEADD | READM;
				Write(flash1,command,1);
				InternalReadMod(flash1, readbits1,1);
				SendNacks(flash1);
				Read(flash1,1);						
				}	
			}*/
	//	Stop(flash1);
	//	SendAcks(flash1);					
	//	printf("Chip ID read in motion sensor is %x\n",readbits1[0]);	

}

//else if(flash1->open == 0)
//printf("CHB trouble\n");
//
//else if(flash->open == 0)
//printf("CHA trouble\n");




//printf("%d is the open in CHA\n%d is the open CHB\n",flash1->open,flash->open);









	



	Close(flash);
//	Close(flash1);


	return 0;
}


