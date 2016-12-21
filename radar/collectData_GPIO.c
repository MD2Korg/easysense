/*
 * Example code of using the low-latency FastRead and FastWrite functions (SPI and C only).
 * Contrast to spiflash.c.
 */
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
int ECGIndex = 0, radarIndex =1; int exitFlag = 0;
//char directoryLocation[] ="/media/sdcard/"; 
//char directoryLocation[] = "/home/code/data/";
char directoryLocation[] = "/home/root/";
/* MD2K Configurations */
// 1 minute = 6000 frames
int TOTAL_FRAMES = 6001; //  We ignore the first fast time frame of the received radar signal
char basename[80];

// ERROR CODES definitions 
#define ERROR_EDISON_SLAVE_RESTART_REQUIRED 0x11
#define ERROR_RADAR_DETECT 0x22
#define ERROR_MOTIONSENSE_DETECT 0x33
#define ERROR_RADAR_OUTPUT_WRITE_DIRECTORY 0x55
#define ERROR_MOTION_SENSE_OUTPUT_WRITE_DIRECTORY 0x66


void * RadarRead()
{
	sleep(3);
	srand(time(NULL));  // seed for random number to select GPIO lines
	
	//increasing the priority of this thread
	struct sched_param schedp;
    	schedp.sched_priority = 1;
        sched_setscheduler(0, SCHED_FIFO, &schedp);
	
	//Timer related variables
	timerEventTrigger *et1 = (timerEventTrigger*)malloc(sizeof(timerEventTrigger)) ;
	struct itimerspec oldRepeatPeriod;
	struct epoll_event ev;
	struct timeval t1,t2,t3,t4,t5,t6;

	//Channel selection variables
	unsigned char flagRandomOrder;
	int numChannels = 16;
	/*int selectionOrder[8]={0,5,10,15,1,6,11,14};*/
//	int selectionOrder[10]={0,1,2,4,5,6,8,9,10,15};

	int selectionOrder[16]={0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15};
	int l;
	int counterRadarSelect =0;
	
	//File handling related variables
	char fileNameConfig[300];
	long int counterFrames=0;
	double fps;
	FILE *fp = NULL,*fp1 = NULL,*fp2 = NULL,*fp3 = NULL;
	char id[80];
	void *retVal;
	retVal = calloc(1,sizeof(int));
	
	char fileName[300],fileNameFinal[310],fileNameFinalSwitch[316],fileNameFPS[316],fileNameSampling[316],fileNameAve[310];	

	sprintf(fileName,"%s%s_Radar.txt",directoryLocation,basename);
	sprintf(fileNameFinal,"%s%s_Radar_Final.txt",directoryLocation,basename);
	sprintf(fileNameFinalSwitch,"%s%s_Radar_Switch.txt",directoryLocation,basename);
	sprintf(fileNameFPS,"%s%s_Radar_FPS.txt",directoryLocation,basename);
	sprintf(fileNameAve,"%s%s_Radar_Ave.bin",directoryLocation,basename);
	fp1 = fopen(fileNameFinalSwitch,"w");
	fp = fopen(fileName,"w");
	fp2 = fopen(fileNameFPS,"w");	
	fp3 = fopen(fileNameAve,"wb"); // file for storing the average value of the received radar signal
	// Checking if the files can be created in the specified location
	if(fp1 == NULL || fp == NULL || fp2 == NULL  || fp4 ==NULL) 
	{
		printf("Error creating output files for radar\n");
		*(int *)retVal =ERROR_RADAR_OUTPUT_WRITE_DIRECTORY; 
		return retVal; 
	}
	
	//MPSSE related variables
	struct mpsse_context *flash = NULL;
	int latencyTimer = 1;
	int i=0;
    
	//Data transfer related variables
	unsigned char readbits;
	unsigned char readbits1[24];
	char command[10] ={0};
	unsigned char dataRead[3000]={0};
	float  dataAve[513] = {0};	

        flash = Open(0x0403,0x6010,SPI0,SIX_MHZ,MSB,IFACE_B,NULL,NULL,latencyTimer ); // reduced the SPI clock speed. 
	if(flash->open)
	{
		// Resetting sweep controller
		command[0] = ResetSweepCtrl | WRITE;
		command[1] = DataLengthCommandStrobe;
		Start(flash);
		FastWrite(flash,command,2);
		Stop(flash);

		//Clears the result of last sweep
		command[0] = ResetCounters | WRITE;
		command[1] = DataLengthCommandStrobe;
		Start(flash);
		FastWrite(flash,command,2);
		Stop(flash);

		//Reading Chip ID	
		command[0] = ChipID | READ;
		command[1] = ChipIDLength;	
		Start(flash);
		FastWrite(flash, command, 2);
		FastRead(flash, readbits1,2);
		Stop(flash);	
		
		printf("Chip ID read is %x\t%x\n",readbits1[0],readbits1[1]);
		//Read the configuration files sent from Matlab.
		// Check if the configuration files are present else return error code	
		if(readConfigFile()==ERROR_CONFIG_FILE_MISSING)
		{
			printf("Configuration file radarConfig missing \n");
			*(int *)retVal = ERROR_CONFIG_FILE_MISSING;	
			return retVal; 
		}

		printf("%d is the frame rate\n",frameRate);		
		//Setting timer related parameters	
		et1->repeatPeriod.it_interval.tv_sec = 0;
		et1->repeatPeriod.it_interval.tv_nsec = (1.0 / frameRate) * 1000000000; // Periodic timer
		et1->repeatPeriod.it_value.tv_sec = 0;
		et1->repeatPeriod.it_value.tv_nsec = (1.0 / frameRate) * 1000000000;// Initial wait period 
		 	
		et1->timerId = timerfd_create(CLOCK_REALTIME,0 );
		et1->epollId = epoll_create1(0);
	
		ev.events = EPOLLIN;
		ev.data.ptr = et1;
		epoll_ctl(et1->epollId,EPOLL_CTL_ADD,et1->timerId,&ev);
		

		// Initialize the radar according to the required configuration
		initializeRadar(flash, command);
		
		// Check if the required values are set in radar registers
		checkInitializationRadar(flash,command);
		

		// Clear Last sweep's results from radar's buffers and output buffers
		clearLastSweepLoadOutput(flash);

		
		// Resetting sweep controller
		command[0] = ResetSweepCtrl | WRITE;
		command[1] = DataLengthCommandStrobe;
		Start(flash);
		FastWrite(flash,command,2);
		Stop(flash);

		// Check if the required values are set in radar registers
		//checkInitializationRadar(flash,command); 
		
		// Begins the timer
		timerfd_settime(et1->timerId,0,&et1->repeatPeriod,&oldRepeatPeriod);	
		gettimeofday(&t4,NULL);	
			
		switchingSequencerModified(flash,selectionOrder[counterRadarSelect] );
		// The main loop		
			while(1)
			{
				counterFrames= counterFrames + 1;	
				//printf("%ld is counter frame\n",counterFrames);	numChannelsnumChannels				
				gettimeofday(&t1,NULL);
				// Start the radar sweep			
				startRadarSweep(flash);	
				
			
				// Ignore the first sweep's data that was done before the timer was started
				if(counterFrames > 1)
				{
				// Reads the previous sweep's data while the current sweep is going on 
				periodicFunc(flash,dataRead);		
					
				}
				// Waits until the current sweep is completed
				monitorSweepStatus(flash);
			
				if(counterFrames >1)
				{	// Writing the data and selected channel index onto a file
									
					fwrite(dataRead,1,2048,fp);		
					processDataAvg(dataRead,dataAve,counterFrames-1);		
				}					
				
							
				fprintf(fp1,"%d\n",counterRadarSelect);	
				
				// Select the next receiver antenna 
				counterRadarSelect = (counterRadarSelect + 1) % numChannels;	
						
				switchingSequencerModified(flash,selectionOrder[counterRadarSelect] );
				// Loads the output buffer with previous sweep's results
				loadOuputBuffer(flash);
				
				// Wait until the required period expires in the timer,
				while(1)
				{		    		
					struct epoll_event ev1;
		    			int nfds = epoll_wait(et1->epollId,&ev1,1,-1);
		    			if(nfds == EINTR) continue;
		   			timerEventTrigger *et2 = (timerEventTrigger*)ev1.data.ptr;
		    			uint64_t exp;
		    			ssize_t s = read(et2->timerId,&exp,sizeof(uint64_t));
		    			if(s != sizeof(uint64_t)) continue;
					break;
				}
				gettimeofday(&t2,NULL);
				
				// Calculating the average frame rate using running average. 
				fps = fps*(counterFrames-1)/counterFrames + (1.0/counterFrames)* 1000.0/((t2.tv_sec - t1.tv_sec)*1000+ (t2.tv_usec - t1.tv_usec)/1000.0);
//				printf("%f is the time taken for Radar\n",((t2.tv_sec - t1.tv_sec)*1000+ (t2.tv_usec - t1.tv_usec)/1000.0));
	//			printf("%f is the frame rate\n",fps);


				// 5 minute = 6000 frames
				if(counterFrames >= TOTAL_FRAMES)
				{
					//printf("Exiting measurement!\n");
					exitFlag = 1;
					break;
				}
				
			}
		
		

	}
	else
	{
		// returning error code if radar is not detected 
		printf("Error in detecting Radar\n");
		*(int *)retVal =ERROR_RADAR_DETECT; 
		return retVal; 
	}
	//printf("%ld is counterFrames\n",counterFrames);
	gettimeofday(&t3,NULL);
	printf("Total Measurement time is %f seconds\n",((t3.tv_sec - t4.tv_sec)*1000+ (t3.tv_usec - t4.tv_usec)/1000.0)/1000.0);
	//printf("Frames per second=%f\n",fps);
	fprintf(fp2,"%f\n%f",((t3.tv_sec - t4.tv_sec)*1000+ (t3.tv_usec - t4.tv_usec)/1000.0)/1000.0,fps);
	Close(flash);
	fwrite(dataAve,4,512,fp3); //storing the average value into a file.	
	fclose(fp);
	fclose(fp1);
	fclose(fp2);
	fclose(fp3);
	// Processes the file that contains the raw bytes and converts into samples by combining 4 bytes.

	//processFile(fileName,fileNameFinal);
	*(int *)retVal =SUCCESS_1; 
	return retVal;

}
void * MotionSenseRead()
{
	sleep(2);
	struct sched_param schedp;
	    schedp.sched_priority = 1;
	        sched_setscheduler(0, SCHED_FIFO, &schedp);
	struct timeval t1,t2,t3,t4,t5,t6;
	char readbits[24] ;
	unsigned char readbits1[24];
	char command[10] ={0};
	char flagAcc = 0;
	long int countSamples = 0;
	struct mpsse_context *flash = NULL;
	/* FIle Handling Variables */	
	FILE *fp = NULL,*fp1 = NULL,*fp2 = NULL,*fp3 = NULL,*fp4 = NULL,*fp5 = NULL,*fp6 = NULL,*fp7= NULL,*fp8=NULL;

	void * retVal;
	retVal = calloc(1,sizeof(int));
	
	
	char fileName[300],fileNameAccX[310],fileNameAccY[316],fileNameAccZ[316],fileNameGyroX[310],fileNameGyroY[316],fileNameGyroZ[316];	
	char fileNameCh1[310],fileNameCh2[316];	
	sprintf(fileNameCh1,"%s%s_Ch1.txt",directoryLocation,basename);
	sprintf(fileNameCh2,"%s%s_Ch2.txt",directoryLocation,basename);
	fp7 = fopen(fileNameCh1,"w");
	fp8 = fopen(fileNameCh2,"w");
	sprintf(fileNameAccX,"%s%s_AccX.txt",directoryLocation,basename);
	sprintf(fileNameAccY,"%s%s_AccY.txt",directoryLocation,basename);
	sprintf(fileNameAccZ,"%s%s_AccZ.txt",directoryLocation,basename);
	sprintf(fileNameGyroX,"%s%s_GyroX.txt",directoryLocation,basename);
	sprintf(fileNameGyroY,"%s%s_GyroY.txt",directoryLocation,basename);
	sprintf(fileNameGyroZ,"%s%s_GyroZ.txt",directoryLocation,basename);	
	fp1 = fopen(fileNameAccX,"w");
	fp2 = fopen(fileNameAccY,"w");
	fp3 = fopen(fileNameAccZ,"w");	
	fp4 = fopen(fileNameGyroX,"w");
	fp5 = fopen(fileNameGyroY,"w");
	fp6 = fopen(fileNameGyroZ,"w");	
	\\Checking if the files can be created in the specified location
	if(fp1 == NULL || fp2 == NULL || fp3 == NULL || fp4 ==NULL || fp5 == NULL|| fp6 == NULL || fp7 == NULL || fp8== NULL)
	{
		printf("Error creating output files for motion sensor and ECG \n");
		*(int *)retVal =  ERROR_MOTION_SENSE_OUTPUT_WRITE_DIRECTORY;
		return retVal; 
	}	

	/*Timer related variables*/
	timerEventTrigger *et1 = (timerEventTrigger*)malloc(sizeof(timerEventTrigger));
	struct epoll_event ev;
	double frameRate = 100;
	struct itimerspec oldRepeatPeriod;


	short dataReadCh1 =0,dataReadCh2 =0,dataReadAccX = 0,dataReadAccY = 0,dataReadAccZ = 0,dataReadGyroX = 0,dataReadGyroY = 0,dataReadGyroZ = 0;

	int latencyTimer = 1;
	int i=0;
    
	int counterRadarSelect =0;
	flash = Open(0x0403,0x6010,I2C,FOUR_HUNDRED_KHZ,MSB,IFACE_A,NULL,NULL,latencyTimer );
	if(flash->open)
	{
		
		et1->repeatPeriod.it_interval.tv_sec = 0;
		et1->repeatPeriod.it_interval.tv_nsec = (1.0 / frameRate) * 1000000000; /* Periodic timer */
		et1->repeatPeriod.it_value.tv_sec = 0;
		et1->repeatPeriod.it_value.tv_nsec = (1.0 / frameRate) * 1000000000;/* Initial wait period */ 
		 
		et1->timerId = timerfd_create(CLOCK_REALTIME,0 );
		et1->epollId = epoll_create1(0);
	
		ev.events = EPOLLIN;
		ev.data.ptr = et1;
		epoll_ctl(et1->epollId,EPOLL_CTL_ADD,et1->timerId,&ev);
		
		command[0] = SLAVEADD1 | WRITEM; // Configuration register write into ADC
		Start(flash);
		Write(flash,command,1);
		if(GetAck(flash) == ACK)
			{
			command[0] = 0x02 | COMMANDMODE;
			command[1] = 0x30;
			Write(flash,command,2);
			}
		Stop(flash);

		
		command[0] = SLAVEADD | WRITEM; // Powering Motion Sensor ON
		Start(flash);
		Write(flash,command,1);
		if(GetAck(flash) == ACK)
			{
			command[0] = 0x6B;
			Write(flash,command,1);
			command[0] = 0x00;
			Write(flash,command,1);
			}
		Stop(flash);
		
		command[0] = SLAVEADD | WRITEM; // Choosing I2C interface
		Start(flash);
		Write(flash,command,1);

		if(GetAck(flash) == ACK)
			{
			command[0] = 0x6A;
			Write(flash,command,1);
			command[0] = 0x00;
			Write(flash,command,1);
			}
		Stop(flash);
		
		command[0] = SLAVEADD | WRITEM; // Reading chip ID
		Start(flash);
		Write(flash,command,1);
		if(GetAck(flash) == ACK)
			{
			command[0] = 0x75;
			
			Write(flash,command,1);
			if(GetAck(flash) == ACK)
				{
				Start(flash);
				command[0] = SLAVEADD | READM;
				Write(flash,command,1);
				InternalReadMod(flash, readbits1,1);
				SendNacks(flash);
				Read(flash,1);						
				}	
			}
		Stop(flash);
		SendAcks(flash);					
		printf("Chip ID read in motion sensor is %x\n",readbits1[0]);	

		command[0] = SLAVEADD | WRITEM; // Set accelaration full scale range
		Start(flash);
		Write(flash,command,1);
		if(GetAck(flash) == ACK)
			{
			command[0] = 0x1C;
			Write(flash,command,1);
			command[0] = 0x00;
			Write(flash,command,1);
			}
		Stop(flash);
		command[0] = SLAVEADD | WRITEM; // Set Gyro full scale range
		Start(flash);
		Write(flash,command,1);
		if(GetAck(flash) == ACK)
			{
			command[0] = 0x1B;
			Write(flash,command,1);
			command[0] = 0x00;	
			Write(flash,command,1);
			}
		Stop(flash);

		command[0] = SLAVEADD | WRITEM; // external synchronization
		Start(flash);
		Write(flash,command,1);
		if(GetAck(flash) == ACK)
			{
			command[0] = 0x1A;
			Write(flash,command,1);
			command[0] = 0x04;
			Write(flash,command,1);
			}
		Stop(flash);

		command[0] = SLAVEADD | WRITEM; // external synchronization
		Start(flash);
		Write(flash,command,1);
		if(GetAck(flash) == ACK)
			{
			command[0] = 0x19;
			Write(flash,command,1);
			command[0] = 0x31;
			Write(flash,command,1);
			}
		Stop(flash);

		command[0] = SLAVEADD | WRITEM; // motion detection bit
		Start(flash);
		Write(flash,command,1);
		if(GetAck(flash) == ACK)
			{
			command[0] = 0x38;
			Write(flash,command,1);
			command[0] = 0x00;
			Write(flash,command,1);
			}
		Stop(flash);

		command[0] = SLAVEADD | WRITEM; // INT_Enable
		Start(flash);
		Write(flash,command,1);
		if(GetAck(flash) == ACK)
			{
			command[0] = 0x1F;
			Write(flash,command,1);
			command[0] = 0x00;
			Write(flash,command,1);
			}
		Stop(flash);

		usleep(1000);
		/* Begins the timer */
		timerfd_settime(et1->timerId,0,&et1->repeatPeriod,&oldRepeatPeriod);	
	// The main loop	
		gettimeofday(&t4,NULL);	
		while(1)

		{
		countSamples = countSamples + 1;
		flagAcc = (flagAcc +1) %2;
		gettimeofday(&t1,NULL);	
		command[0] = SLAVEADD1 | WRITEM; // Reading Conversoin result in mode 2 from ADC
		command[1] = 0x00 | COMMANDMODE; // Address pointer for conversion result		
		Start(flash);
		Write(flash,command,2);	
		if(GetAck(flash) == ACK)
			{
			Start(flash);			
			command[0] = SLAVEADD1 | READM;			
			Write(flash,command,1);
			if(GetAck(flash) == ACK)
				{				
				InternalReadMod(flash, readbits1, 4);
				SendNacks(flash);
				Read(flash,1);						
				}	
			}
		Stop(flash);
		SendAcks(flash);					
		
		dataReadCh1 = ((readbits1[0]& 0x0F) << 8) | readbits1[1];
		dataReadCh2 = ((readbits1[2]& 0x0F) << 8) | readbits1[3];
		fprintf(fp7,"%d\n", dataReadCh1);	 
		fprintf(fp8,"%d\n", dataReadCh2);
		//if(flagAcc == 1)	
			{
			command[0] = SLAVEADD | WRITEM; // Reading Accelorometer 3 axis data
			command[1] = 0x3B;
			Start(flash);
			Write(flash,command,2);

			Start(flash);
			command[0] = SLAVEADD | READM;
			Write(flash,command,1);
			InternalReadMod(flash, readbits1,14);
			SendNacks(flash);
			Read(flash,1);						
			Stop(flash);

			SendAcks(flash);					
		
		        dataReadAccX = (readbits1[0] << 8) | readbits1[1];
		        if((dataReadAccX & 0x1000) == 0x1000)							dataReadAccX = -(~(dataReadAccX) + 1 );	

													dataReadAccY = (readbits1[2] << 8) | readbits1[3];
		        if((dataReadAccY & 0x1000) == 0x1000)
				dataReadAccY = -(~(dataReadAccY) + 1);
													dataReadAccZ = (readbits1[4] << 8) | readbits1[5];
			if((dataReadAccZ & 0x1000) == 0x1000)
				dataReadAccZ = -(~(dataReadAccZ) + 1);
																							dataReadGyroX = (readbits1[8] << 8) | readbits1[9];
			if((dataReadGyroX & 0x1000) == 0x1000)
				dataReadGyroX = -(~(dataReadGyroX) + 1);
													dataReadGyroY = (readbits1[10] << 8) | readbits1[11];
			if((dataReadGyroY & 0x1000) == 0x1000)
				dataReadGyroY = -(~(dataReadGyroY) + 1);
													dataReadGyroZ = (readbits1[12] << 8) | readbits1[13];
			if((dataReadGyroZ & 0x1000) == 0x1000)
				dataReadGyroZ = -(~(dataReadGyroZ) + 1);



			fprintf(fp4,"%d\n", dataReadGyroX);	 
			fprintf(fp5,"%d\n", dataReadGyroY);
			fprintf(fp6,"%d\n", dataReadGyroZ);
			fprintf(fp1,"%d\n", dataReadAccX);	 
			fprintf(fp2,"%d\n", dataReadAccY);
			fprintf(fp3,"%d\n", dataReadAccZ);
			}
		/*if(flagAcc == 0)
			{
			command[0] = SLAVEADD | WRITEM; // Reading Gyro 3 axis data
			command[1] = 0x43;
			Start(flash);
			Write(flash,command,2);
			Start(flash);
			command[0] = SLAVEADD | READM;
			Write(flash,command,1);
			InternalReadMod(flash, readbits1,6);
			SendNacks(flash);
			Read(flash,1);						
				
			Stop(flash);
			SendAcks(flash);					
		
			dataReadGyroX = (readbits1[0] << 8) | readbits1[1];
			dataReadGyroY = (readbits1[2] << 8) | readbits1[3];
			dataReadGyroZ = (readbits1[4] << 8) | readbits1[5];
			fprintf(fp4,"%d\n", dataReadGyroX);	 
			fprintf(fp5,"%d\n", dataReadGyroY);
			fprintf(fp6,"%d\n", dataReadGyroZ);
			}*/
		gettimeofday(&t5,NULL);

		
		
		// Wait until the required period expires in the timer,
		while(1)
		{			
			struct epoll_event ev1;
			int nfds = epoll_wait(et1->epollId,&ev1,1,-1);
			if(nfds == EINTR) continue;
			timerEventTrigger *et2 = (timerEventTrigger*)ev1.data.ptr;
			uint64_t exp;
			ssize_t s = read(et2->timerId,&exp,sizeof(uint64_t));
			if(s != sizeof(uint64_t)) continue;
			break;
		}	


		gettimeofday(&t2,NULL);		
		//printf("%f is the time taken for Motion Sensor and ADC\n",((t5.tv_sec - t1.tv_sec)*1000+ (t5.tv_usec - t1.tv_usec)/1000.0));
//		printf("%f is the periodic time taken \n",((t2.tv_sec - t1.tv_sec)*1000+ (t2.tv_usec - t1.tv_usec)/1000.0));
				
		if(countSamples >=TOTAL_FRAMES -1)
			{
			//printf("Exiting measurement!\n");
			break;
			}
		}

	}
	else
	{
		// return error code if the motion sensor and ADC are not detected
		printf("Unable to detect Motion sense and ECG\n");
		*(int *)retVal =  ERROR_MOTIONSENSE_DETECT;
		return retVal;
	}

	//printf("%d is the number of screw ups\n",countExceed);
	//gettimeofday(&t3,NULL);
	//printf("Total Measurement time is %f seconds\n",((t3.tv_sec - t4.tv_sec)*1000+ (t3.tv_usec - t4.tv_usec)/1000.0)/1000.0);
	Close(flash);
	fclose(fp1);
	fclose(fp2);
	fclose(fp3);
	fclose(fp4);
	fclose(fp5);
	fclose(fp6);
	fclose(fp7);
	fclose(fp8);
	*(int *)retVal = SUCCESS_1;
	return retVal;



}
// Creating the GPIO file handles for Edison to turn on the power supply
void createport(unsigned char portNum)
{
char command[400];
(sprintf(command,"echo %d > /sys/class/gpio/export",portNum )  );
system(command);
}
// Deleting the GPIO file handles for Edison 
void deleteport(unsigned char portNum)
{
char command[400];
sprintf(command,"[ -d /sys/class/gpio/gpio%d ] && echo %d > /sys/class/gpio/unexport",portNum,portNum );
system(command);
}
//Configuring the GPIO ports of Edison
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
// Setting the register values for configuring GPIO of Edison
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
	unsigned char gpio_en_edison = 47;
	unsigned char gpio_pwm_haptic = 12;
	struct mpsse_context *flash=NULL;

	void  *retVal;
	retVal = calloc(1,sizeof(int));

	deleteport(gpio_devicePower);
	deleteport(gpio_RF_vcc);
	
	deleteport(gpio_analog_vcc);	
	deleteport(gpio_en_edison);	
	deleteport(gpio_pwm_haptic);	
	
	
	createport(gpio_devicePower );
	createport(gpio_RF_vcc);
	createport(gpio_analog_vcc);
	createport(gpio_en_edison);
	createport(gpio_pwm_haptic);
	
        setupPort(gpio_devicePower,1);
	setupPort(gpio_RF_vcc,1);
	setupPort(gpio_analog_vcc,1);
	setupPort(gpio_en_edison,1);
	setupPort(gpio_pwm_haptic,1);

	setValue(gpio_devicePower,1);
	setValue(gpio_RF_vcc,1);
	setValue(gpio_analog_vcc,1);
	sleep(5);
        flash = Open(0x0403,0x6010,GPIO,0,0,IFACE_B,NULL,NULL,1 );
	if( flash->open)
	{
		PinHigh(flash,GPIOH0);
		sleep(10);
		PinLow(flash,GPIOH0);
	}
	else
	{
		printf("something wrong in openning device\n");
		*(int *)retVal = ERROR_EDISON_SLAVE_RESTART_REQUIRED;	 
		return *(int *)retVal;
		//system("reboot");
	}
	Close(flash);
	
	// Sets the priority of the current process to the highest level.
	setpriority(PRIO_PROCESS, 0, -20);	
	

	if (argc >= 3) {
		TOTAL_FRAMES = atoi(argv[2]); //Read in number of frames to sample
	}
	if (argc >= 2) {
			printf("Base filenames: %s\n", argv[1]);
			strcpy(basename, argv[1]);
	} else {
			printf("Default filenames: EasySense\n");
			strcpy(basename,"EasySense");
	}
	
	//MPSSE related variables
	int radarFoundFlag = 0,ADCFoundFlag = 0;
	int latencyTimer = 1;

	

	unsigned char readbits;
	unsigned char readbits1[24];
	char command[10] ={0};
	pthread_t tid,tid1;

	pthread_create(&tid, NULL, MotionSenseRead, NULL);
	pthread_create(&tid1, NULL, RadarRead, NULL);

/*Thread Creation for Radar sampling and ECG sampling */	
    	pthread_join(tid,&retVal ); 
    	pthread_join(tid1, &retVal);	

	setValue(gpio_RF_vcc,0 );
	setValue(gpio_analog_vcc,0);
	setValue(gpio_en_edison,0);


	



	return *(int *)retVal;
}




