/*
 * Example code of using the low-latency FastRead and FastWrite functions (SPI and C only).
 * Contrast to spiflash.c.
 */

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

int main(void)
{
	// Sets the priority of the current process to the highest level.
	setpriority(PRIO_PROCESS, 0, -20);	
	//Timer related variables
	timerEventTrigger *et1 = (timerEventTrigger*)malloc(sizeof(timerEventTrigger))  ;
	struct itimerspec oldRepeatPeriod;
	struct epoll_event ev;
	struct timeval t1,t2,t3,t4,t5,t6;
	int counterKbHit = 0;
	et1->repeatPeriod.it_interval.tv_sec = 0;
	et1->repeatPeriod.it_interval.tv_nsec = 10000000; // Periodic timer
	et1->repeatPeriod.it_value.tv_sec = 0;
	et1->repeatPeriod.it_value.tv_nsec = 10000000;// Initial wait period 
	 	
	et1->timerId = timerfd_create(CLOCK_REALTIME,0 );
	et1->epollId = epoll_create1(0);
	
	ev.events = EPOLLIN;
	ev.data.ptr = et1;
	epoll_ctl(et1->epollId,EPOLL_CTL_ADD,et1->timerId,&ev);

	//File handling related variables
	char fileNameConfig[300];
	long int counterFrames=0;
	double fps;
	FILE *fp = NULL,*fp1 = NULL,*fp2 = NULL;
	char id[80];
	fp = fopen("LastFileName.txt","r");	
	fscanf(fp,"%s",id); 
	fclose(fp);	
	char fileName[300],fileNameFinal[310],fileNameFinalSwitch[316],fileNameFPS[316];	
	sprintf(fileName,"%s.txt",id);
	sprintf(fileNameFinal,"%s_Final.txt",id);
	sprintf(fileNameFinalSwitch,"%s_Switch.txt",id);
	sprintf(fileNameFPS,"%s_FPS.txt",id);
	fp1 = fopen(fileNameFinalSwitch,"w");
	fp = fopen(fileName,"w");
	fp2 = fopen(fileNameFPS,"w");

	//MPSSE related variables
	struct mpsse_context *flash = NULL;
	int latencyTimer = 1;
	int i=0;
    
	unsigned char readbits;
	unsigned char readbits1[24];
	char command[10] ={0};
	unsigned char dataRead[3000]={0};
	
	int counterRadarSelect =0;
	fflush(stdin);
	
	//printf("%c is in stdin\n",ch);
    	//while ((ch = fgetc(stdin)) != EOF ) {
        /* null body */
    					//}
	if((flash = MPSSE(SPI0, TWELVE_MHZ , MSB,latencyTimer)) != NULL && flash->open)
	{
		
		// Setting the GPIO1 high in all SPI states.
		flash->pidle = (flash->pidle | GPIO1);
		flash->pstart = (flash->pstart | GPIO1);
		flash->pstop = (flash->pstop | GPIO1);
		printf("Okay, Radar is connected\n");

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
		
		//Read the configuration files sent from Matlab.
		//readConfigFile();	
		// Initialize the radar according to the required configuration
		//initializeRadar(flash, command);
		// Check if the required values are set in radar registers
		//checkInitializationRadar(flash,command); 

		printf("Press ENTER to stop the measurement process\n");
		// Clear Last sweep's results from radar's buffers and output buffers
		clearLastSweepLoadOutput(flash);
		// Start new Radar sweep
		//startRadarSweep(flash);
		// A blocking function that waits until sweep status indicates completion.
		//monitorSweepStatus(flash);
		// Loads the output buffer which is a double buffer for radar's output with the current sweep's results
		//loadOuputBuffer(flash);
		// Begins the timer
		timerfd_settime(et1->timerId,0,&et1->repeatPeriod,&oldRepeatPeriod);	
		gettimeofday(&t4,NULL);	
		
		// The main loop		
		while(1)
		{
			counterFrames= counterFrames + 1;								
			gettimeofday(&t1,NULL);
			// Start the radar sweep			
			//startRadarSweep(flash);		
			// Ignore the first sweep's data that was done before the timer was started
			if(counterFrames > 1)
			{
			// Reads the previous sweep's data while the current sweep is going on 
			//periodicFunc(flash,dataRead);
			// Writing the data onto a file
			fwrite(dataRead,1,2048,fp);				
			}
			// Waits until the current sweep is completed
			//monitorSweepStatus(flash);

			// Select the next receiver antenna 
			//fprintf(fp1,"%d\n",counterRadarSelect);		
			switchingSequencer(flash, &counterRadarSelect);
			// Loads the output buffer with previous sweep's results
			loadOuputBuffer(flash);

			// Wait until the required period expires in the timer,
		    	struct epoll_event ev1;
		    	int nfds = epoll_wait(et1->epollId,&ev1,1,-1);
		    	if(nfds == EINTR) continue;
		   	timerEventTrigger *et2 = (timerEventTrigger*)ev1.data.ptr;
		    	uint64_t exp;
		    	ssize_t s = read(et2->timerId,&exp,sizeof(uint64_t));
		    	if(s != sizeof(uint64_t)) continue;

			gettimeofday(&t2,NULL);
			// Calculating the average frame rate using running average. 
			fps = fps*(counterFrames-1)/counterFrames + (1.0/counterFrames)* 1000.0/((t2.tv_sec - t1.tv_sec)*1000+ (t2.tv_usec - t1.tv_usec)/1000.0); 
			//printf("%f is the time taken\n",((t2.tv_sec - t1.tv_sec)*1000+ (t2.tv_usec - t1.tv_usec)/1000.0));
			//printf("%f is the frame rate\n",fps);
			t1=t2;
			// Take care of keypress event.
			if(kbhit())
			{
				
							
					
					printf("Exiting measurement!\n");
					break;
					
			}
		}
	}
	else
		printf("Something's wrong!\n");

	printf("%ld is counterFrames\n",counterFrames);
	gettimeofday(&t3,NULL);
	printf("Total Measurement time is %f seconds\n",((t3.tv_sec - t4.tv_sec)*1000+ (t3.tv_usec - t4.tv_usec)/1000.0)/1000.0);
	printf("Frames per second=%f\n",fps);
	fprintf(fp2,"%f\n%f",((t3.tv_sec - t4.tv_sec)*1000+ (t3.tv_usec - t4.tv_usec)/1000.0)/1000.0,fps);
	Close(flash);
	fclose(fp);
	fclose(fp1);
	fclose(fp2);
	// Processes the file that contains the raw bytes and converts into samples by combining 4 bytes.
	//processFile(fileName,fileNameFinal);

	return 0;
}


