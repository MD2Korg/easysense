#include<stdlib.h>
#include<time.h>
#include<unistd.h>
#include<inttypes.h>
#include"mpsse.h"
#include<stdio.h>
#include<string.h>
#include<sys/resource.h>
#include<sys/timerfd.h>
#include<sys/epoll.h>
#include<errno.h>



/* MD2K Configurations */

#define RADAR_CONFIG "radarConfig"





/*Structure for event asociated with a timer typedef struct*/ 
typedef struct {
	struct itimerspec repeatPeriod;
	int timerId;
	int epollId;
}timerEventTrigger;

#define READ 0x00	/*Address is Or'ed with this in case of read operation*/
#define WRITE 0x80	/*Address is Or'ed with this in case of write operation*/

/*Command Strobes Register address*/
#define ResetSweepCtrl 0x44
#define StartSweep 0x43
#define ResetCounters 0x25
#define LoadOutputBuffer 0x24

/*Length of data during action stobe*/
#define DataLengthCommandStrobe 0x00

/*variable for Frame Rate*/
unsigned char frameRate = 100;
/*values for Normalization */
double iterationsVal,avgFactorVal,dacMinVal,dacStepVal;

/*Register addresses*/
#define ChipID 0x02
#define ChipIDLength 0x02
#define SampleReadOutCtrl 0x21
#define SampleReadOutCtrlLength 0x02                 
unsigned char SampleReadOutCtrlData_0 = 0x0F;
unsigned char SampleReadOutCtrlData_1 = 0x80; /* Downsampling is set to 0x00 and will be read from configuration file */
#define SampleCtrl 0x22
#define SampleCtrlLength 0x01
#define SampleCtrlData_0 0x00
#define ThresholdPowerDown 0x23 
#define ThresholdPowerDownLength 0x01
#define ThresholdPowerDownData_0 0x01
#define ThresholdPowerDownData_0_Shutdown 0x03
#define SampleInputCtrl 0x26
#define SampleInputCtrlLength 0x01
#define SampleInputCtrlData_0 0x00
#define ThresholdCtrl 0x27
#define ThresholdCtrlLength 0x01
unsigned char ThresholdCtrlData_0 = 0x00; /* Gain is set to 0x00 and will be read from configuration file*/
#define FocusPulsesPerStep 0x30
#define FocusPulsesPerStepLength 0x04
unsigned char FocusPulsesPerStepData_0 = 0x00; /* Pulses per step are set to 0 and are read from configuration file */
unsigned char FocusPulsesPerStepData_1 = 0x00;
unsigned char FocusPulsesPerStepData_2 = 0x00;
unsigned char FocusPulsesPerStepData_3 = 0x00;
#define NormalPulsesPerStep 0x31
#define NormalPulsesPerStepLength 0x04
unsigned char NormalPulsesPerStepData_0 = 0x00;
unsigned char NormalPulsesPerStepData_1 = 0x00;
unsigned char NormalPulsesPerStepData_2 = 0x00;
unsigned char NormalPulsesPerStepData_3 = 0x00;
#define DACFirstIterationSetupTime 0x32
#define DACFirstIterationSetupTimeLength 0x02
#define DACFirstIterationSetupTimeData_0 0x00
#define DACFirstIterationSetupTimeData_1 0x00
#define DACFirstStepSetupTime 0x33
#define DACFirstStepSetupTimeLength 0x02
#define DACFirstStepSetupTimeData_0 0x00
#define DACFirstStepSetupTimeData_1 0x00
#define DACRegularSetupTime 0x34
#define DACRegularSetupTimeLength 0x02
#define DACRegularSetupTimeData_0 0x00
#define DACRegularSetupTimeData_1 0x00
#define DACLastIterationHoldTime 0x35
#define DACLastIterationHoldTimeLength 0x02
#define DACLastIterationHoldTimeData_0 0x00
#define DACLastIterationHoldTimeData_1 0x00
#define DACLastStepHoldTime 0x36
#define DACLastStepHoldTimeLength 0x02
#define DACLastStepHoldTimeData_0 0x00
#define DACLastStepHoldTimeData_1 0x00
#define DACRegularHoldTime 0x37
#define DACRegularHoldTimeLength 0x02
#define DACRegularHoldTimeData_0 0x00
#define DACRegularHoldTimeData_1 0x00
#define SweepMainCtrl 0x38
#define SweepMainCtrlLength 0x01
#define SweepMainCtrlData_0 0x1E /* The datasheet is given wrong we have to set SweepPhase=1, and ManualSweepDir=1 */
#define DACMax 0x39
#define DACMaxLength 0x02
unsigned char DACMaxData_0 = 0x00;
unsigned char DACMaxData_1 = 0x00;
#define DACMin 0x3A
#define DACMinLength 0x02
unsigned char DACMinData_0 = 0x00;
unsigned char DACMinData_1 = 0x00;
#define DACstep 0x3B
#define DACstepLength 0x02
unsigned char DACstepData_0 = 0x00;
unsigned char DACstepData_1 = 0x00;
#define Iterations 0x3C
#define IterationsLength 0x02
unsigned char IterationsData_0 = 0x00;
unsigned char IterationsData_1 = 0x00;
#define FocusMax 0x3D
#define FocusMaxLength 0x02
unsigned char FocusMaxData_0 = 0x00;
unsigned char FocusMaxData_1 = 0x00;
#define FocusMin 0x3E
#define FocusMinLength 0x02
unsigned char FocusMinData_0 = 0x00;
unsigned char FocusMinData_1 = 0x00;
#define FocusSetupTime 0x40
#define FocusSetupTimeLength 0x01
#define FocusSetupTimeData_0 0x00
#define FocusHoldTime 0x41
#define FocusHoldTimeLength 0x01
#define FocusHoldTimeData_0 0x00
#define SweepClkCtrl 0x42
#define SweepClkCtrlLength 0x01
#define SweepClkCtrlData_0 0x00
#define PulseGeneratorCtrl 0x50
#define PulseGeneratorCtrlLength 0x01
 #define PulseGeneratorCtrlData_0 0x42  /*low band + nominal */
/*#define PulseGeneratorCtrlData_0 0x80  Medium + slow 
 #define PulseGeneratorCtrlData_0 0x02*/ /* Turn off */ 
 #define DACCtrl 0x58
#define DACCtrlLength 0x02
#define DACCtrlData_0 0x80
#define DACCtrlData_1 0x00
#define MCLKCtrl 0x60
#define MCLKCtrlLength 0x01
#define MCLKCtrlData_0 0x10
#define StaggeredPRFCtrl 0x61
#define StaggeredPRFCtrlLength 0x01
#define StaggeredPRFCtrlData_0 0x04
#define StaggeredPRFDelay 0x62
#define StaggeredPRFDelayLength 0x01
#define StaggeredPRFDelayData_0 0x00
#define LFSR5TapEnable 0x64
#define LFSR5TapEnableLength 0x02
#define LFSR5TapEnableData_0 0x50
#define LFSR5TapEnableData_1 0x08
#define LFSR4TapEnable 0x65
#define LFSR4TapEnableLength 0x02
#define LFSR4TapEnableData_0 0x4A
#define LFSR4TapEnableData_1 0x00
#define LFSR3TapEnable 0x66
#define LFSR3TapEnableLength 0x02
#define LFSR3TapEnableData_0 0x48
#define LFSR3TapEnableData_1 0x01
#define LFSR2TapEnable 0x67
#define LFSR2TapEnableLength 0x02
#define LFSR2TapEnableData_0 0x41
#define LFSR2TapEnableData_1 0x20
#define LFSR1TapEnable 0x68
#define LFSR1TapEnableLength 0x02
#define LFSR1TapEnableData_0 0x41
#define LFSR1TapEnableData_1 0x08
#define LFSR0TapEnable 0x69
#define LFSR0TapEnableLength 0x02
#define LFSR0TapEnableData_0 0x41
#define LFSR0TapEnableData_1 0x20
#define TimingCtrl 0x6A
#define TimingCtrlLength 0x01
#define TimingCtrlData_0 0x00
#define SampleDelayCoarseTune 0x6B
#define SampleDelayCoarseTuneLength 0x02
unsigned char SampleDelayCoarseTuneData_0 = 0x00; /* Read from Configuration file */
unsigned char SampleDelayCoarseTuneData_1 = 0x00; 
#define SampleDelayMediumTune 0x6C
#define SampleDelayMediumTuneLength 0x01
unsigned char SampleDelayMediumTuneData_0 = 0x00; /* Read from configuration file */
#define SampleDelayFineTune 0x6D
#define SampleDelayFineTuneLength 0x01
#define SampleDelayFineTuneData_0 0x00
#define SendPulseDelayCoarseTune 0x6E
#define SendPulseDelayCoarseTuneLength 0x02
#define SendPulseDelayCoarseTuneData_0 0x00
#define SendPulseDelayCoarseTuneData_1 0x00
#define SendPulseDelayMediumTune 0x6F
#define SendPulseDelayMediumTuneLength 0x01
#define SendPulseDelayMediumTuneData_0 0x00
#define SendPulseDelayFineTune 0x70
#define SendPulseDelayFineTuneLength 0x01
#define SendPulseDelayFineTuneData_0 0x00
#define TimingCalibrationCtrl 0x71
#define TimingCalibrationCtrlLength 0x01
#define TimingCalibrationCtrlData_0 0x00
#define MCLKOutputCtrl 0x75
#define MCLKOutputCtrlLength 0x01
#define MCLKOutputCtrlData_0 0x01
#define SweepControllerStatus 0x47
#define SweepControllerStatusLength 0x02
#define SampleOutputBuffer 0x20
#define SampleOutputBufferStartLength 0x7F
#define SampleOutputBufferContinueLength 0xFF
#define SampleOutputBufferFinalLength 0x90


// Error code for configuration files
#define ERROR_CONFIG_FILE_MISSING 0x44
#define SUCCESS_1 0xFF

/* Function declarations */
int readConfigFile();
void initializeRadar(struct mpsse_context *, char *);	
int checkInitializationRadar(struct mpsse_context * , char *);
int initializeCheckRadar(struct mpsse_context * , char *);
int kbhit();
void periodicFunc(struct mpsse_context * ,unsigned char* );
void processFile(char *, char *);
void monitorSweepStatus(struct mpsse_context * );
void clearLastSweepLoadOutput(struct mpsse_context * );
void loadOuputBuffer(struct mpsse_context * );
void startRadarSweep(struct mpsse_context * );
void switchingSequencer(struct mpsse_context * , int * t);
void switchingSequencerCalib(struct mpsse_context * , int *);
void swap(int *,int , int  );
void shuffle(int *, int  );
void processData(unsigned char *buffer,float *buffer1);
void switchingSequencerModified(struct mpsse_context * flash, int radarSelect);
void processDataAvg(unsigned char *buffer, float *buffer1, int counterFrames);
/* Function definitions */

/*Configuration settings of all the radar registers*/
void initializeRadar(struct mpsse_context * flash, char *command)
{
	unsigned char readbits1[24];
	/*Writing into SampleReadOutCtrl*/
	command[0] = SampleReadOutCtrl | WRITE;
	command[1] = SampleReadOutCtrlLength;
	command[2] = SampleReadOutCtrlData_0;
	command[3] = SampleReadOutCtrlData_1;	
	Start(flash);
	FastWrite(flash,command,4);
	Stop(flash);
	
	/*Writing into SamplerCtrl*/
	command[0] = SampleCtrl | WRITE;
	command[1] = SampleCtrlLength;
	command[2] = SampleCtrlData_0;	
	Start(flash);
	FastWrite(flash,command,3);
	Stop(flash);

	/*Writing into ThresholdPowerDown*/
	command[0] = ThresholdPowerDown | WRITE;
	command[1] = ThresholdPowerDownLength;
	command[2] = ThresholdPowerDownData_0;
	Start(flash);
	FastWrite(flash,command,3);
	Stop(flash);

	/*Writing into SampleInputControl*/
	command[0] = SampleInputCtrl | WRITE;
	command[1] = SampleInputCtrlLength;
	command[2] = SampleInputCtrlData_0;
	Start(flash);
	FastWrite(flash,command,3);
	Stop(flash);

	/*Writing into ThresholdCtrl*/
	command[0] = ThresholdCtrl | WRITE;
	command[1] = ThresholdCtrlLength;
	command[2] = ThresholdCtrlData_0;		
	Start(flash);
	FastWrite(flash,command,3);
	Stop(flash);
		

	/*Writing into NormalPulsesPerStep*/
	command[0] = NormalPulsesPerStep | WRITE;
	command[1] = NormalPulsesPerStepLength; 
	command[2] = NormalPulsesPerStepData_0;
	command[3] = NormalPulsesPerStepData_1;
	command[4] = NormalPulsesPerStepData_2;
	command[5] = NormalPulsesPerStepData_3;
	Start(flash);
	FastWrite(flash,command,6);
	Stop(flash);

	
	/*Writing into FocusPulsesPerStep*/
	command[0] = FocusPulsesPerStep | WRITE;
	command[1] = FocusPulsesPerStepLength;
	command[2] = FocusPulsesPerStepData_0;
	command[3] = FocusPulsesPerStepData_1;
	command[4] = FocusPulsesPerStepData_2;
	command[5] = FocusPulsesPerStepData_3;		
	Start(flash);
	FastWrite(flash,command,6);
	Stop(flash);


	/*Writing into DACFirstIterationSetupTime*/
	command[0] = DACFirstIterationSetupTime | WRITE; 
	command[1] = DACFirstIterationSetupTimeLength;
	command[2] = DACFirstIterationSetupTimeData_0;
	command[3] = DACFirstIterationSetupTimeData_1; 
	Start(flash);
	FastWrite(flash,command,4);
	Stop(flash);

	
	/*Writing into DACFirstStepSetupTime*/ 
	command[0] = DACFirstStepSetupTime | WRITE;
	command[1] = DACFirstStepSetupTimeLength;
	command[2] = DACFirstStepSetupTimeData_0;
	command[3] = DACFirstStepSetupTimeData_1;
	Start(flash);
	FastWrite(flash,command,4);
	Stop(flash);
	
	/*Writing into DACRegularSetupTime */
	command[0] = DACRegularSetupTime | WRITE;
	command[1] = DACRegularSetupTimeLength;
	command[2] = DACRegularSetupTimeData_0;
	command[3] = DACRegularSetupTimeData_1; 
	Start(flash);
	FastWrite(flash,command,4);
	Stop(flash);	

	/*Writing into DACLastIterationHoldTime*/ 
	command[0] = DACLastIterationHoldTime | WRITE;
	command[1] = DACLastIterationHoldTimeLength;
	command[2] = DACLastIterationHoldTimeData_0;
	command[3] = DACLastIterationHoldTimeData_1; 
	Start(flash);
	FastWrite(flash,command,4);
	Stop(flash);
			

	/*Writing into DACLastStepHoldTime */
	command[0] = DACLastStepHoldTime | WRITE;
	command[1] = DACLastStepHoldTimeLength;
	command[2] = DACLastStepHoldTimeData_0;
	command[3] = DACLastStepHoldTimeData_1;
	Start(flash);
	FastWrite(flash,command,4);
	Stop(flash);


	/*Writing into DACRegularHoldTime */
	command[0] = DACRegularHoldTime | WRITE; 
	command[1] = DACRegularHoldTimeLength;
	command[2] = DACRegularHoldTimeData_0;
	command[3] = DACRegularHoldTimeData_1;
	Start(flash);
	FastWrite(flash,command,4);
	Stop(flash);


	/*Writing into SweepMainCtrl*/
	command[0] = SweepMainCtrl | WRITE;
	command[1] = SweepMainCtrlLength;
	command[2] = SweepMainCtrlData_0;
	Start(flash);
	FastWrite(flash,command,3);
	Stop(flash);


	/*Writing into DACMax*/
	command[0] = DACMax | WRITE;
	command[1] = DACMaxLength;
	command[2] = DACMaxData_0;
	command[3] = DACMaxData_1;
	Start(flash);
	FastWrite(flash,command,4);
	Stop(flash);


	/*Writing into DACMin*/
	command[0] = DACMin | WRITE;
	command[1] = DACMinLength;
	command[2] = DACMinData_0;
	command[3] = DACMinData_1;	
	Start(flash);
	FastWrite(flash,command,4);
	Stop(flash);
	

	/*Writing into DACstep*/
	command[0] = DACstep | WRITE;
	command[1] = DACstepLength;
	command[2] = DACstepData_0;
	command[3] = DACstepData_1;
	Start(flash);
	FastWrite(flash,command,4);
	Stop(flash);


	/*Writing into Iterations*/
	command[0] = Iterations | WRITE;
	command[1] = IterationsLength;
	command[2] = IterationsData_0;
	command[3] = IterationsData_1;
	Start(flash);
	FastWrite(flash,command,4);
	Stop(flash);

	
	/*Writing into FocusMax*/
	command[0] = FocusMax | WRITE;
	command[1] = FocusMaxLength;
	command[2] = FocusMaxData_0;
	command[3] = FocusMaxData_1;
	Start(flash);
	FastWrite(flash,command,4);
	Stop(flash);
	

	/*Writing into FocusMin*/
	command[0] = FocusMin | WRITE;
	command[1] = FocusMinLength;
	command[2] = FocusMinData_0;
	command[3] = FocusMinData_1;
	Start(flash);
	FastWrite(flash,command,4);
	Stop(flash);


	/*Writing into FocusSetupTime*/
	command[0] = FocusSetupTime | WRITE;
	command[1] = FocusSetupTimeLength;
	command[2] = FocusSetupTimeData_0;
	Start(flash);
	FastWrite(flash,command,3);
	Stop(flash);

	/*Writing into FocusHoldTime*/
	command[0] = FocusHoldTime | WRITE;
	command[1] = FocusHoldTimeLength;
	command[2] = FocusHoldTimeData_0;
	Start(flash);
	FastWrite(flash,command,3);
	Stop(flash);
		

	/*Writing into SweepClkCtrl*/
	command[0] = SweepClkCtrl | WRITE;
	command[1] = SweepClkCtrlLength;
	command[2] = SweepClkCtrlData_0;		
	Start(flash);
	FastWrite(flash,command,3);
	Stop(flash);

	/*Writing into PulseGeneratorCtrl*/
	command[0] = PulseGeneratorCtrl | WRITE;
	command[1] = PulseGeneratorCtrlLength;
	command[2] = PulseGeneratorCtrlData_0;
	Start(flash);
	FastWrite(flash,command,3);
	Stop(flash);

	/*Writing into DACCtrl*/
	command[0] = DACCtrl | WRITE;
	command[1] = DACCtrlLength; 
	command[2] = DACCtrlData_0;
	command[3] = DACCtrlData_1; 		
	Start(flash);
	FastWrite(flash,command,4);
	Stop(flash);

	/*Writing into MCLKCtrl*/
	command[0] = MCLKCtrl | WRITE;
	command[1] = MCLKCtrlLength;
	command[2] = MCLKCtrlData_0;
	Start(flash);
	FastWrite(flash,command,3);
	Stop(flash);
	
	/*Writing into StaggeredPRFCtrl*/
	command[0] = StaggeredPRFCtrl | WRITE;
	command[1] = StaggeredPRFCtrlLength;
	command[2] = StaggeredPRFCtrlData_0;
	Start(flash);
	FastWrite(flash,command,3);
	Stop(flash);

	/*Writing into StaggeredPRFCtrlDelay*/
	command[0] = StaggeredPRFDelay | WRITE;		
	command[1] = StaggeredPRFDelayLength;
	command[2] = StaggeredPRFDelayData_0;
	Start(flash);
	FastWrite(flash,command,3);
	Stop(flash);

	/*Writing into LFSR5TapEnable*/
	command[0] = LFSR5TapEnable | WRITE;
	command[1] = LFSR5TapEnableLength;
	command[2] = LFSR5TapEnableData_0;
	command[3] = LFSR5TapEnableData_1;	
	Start(flash);
	FastWrite(flash,command,4);
	Stop(flash);
		
	/*Writing into LFSR4TapEnable*/
	command[0] = LFSR4TapEnable | WRITE;
	command[1] = LFSR4TapEnableLength;
	command[2] = LFSR4TapEnableData_0;
	command[3] = LFSR4TapEnableData_1;
	Start(flash);
	FastWrite(flash,command,4);
	Stop(flash);
		
	/*Writing into LFSR3TapEnable*/
	command[0] = LFSR3TapEnable | WRITE;
	command[1] = LFSR3TapEnableLength;
	command[2] = LFSR3TapEnableData_0;
	command[3] = LFSR3TapEnableData_1; 
	Start(flash);
	FastWrite(flash,command,4);
	Stop(flash);
	
	/*Writing into LFSR2TapEnable*/
	command[0] = LFSR2TapEnable | WRITE;
	command[1] = LFSR2TapEnableLength;
	command[2] = LFSR2TapEnableData_0;
	command[3] = LFSR2TapEnableData_1;
	Start(flash);
	FastWrite(flash,command,4);
	Stop(flash);
	
	/*Writing into LFSR1TapEnable*/
	command[0] = LFSR1TapEnable | WRITE;
	command[1] = LFSR1TapEnableLength;
	command[2] = LFSR1TapEnableData_0;
	command[3] = LFSR1TapEnableData_1;
	Start(flash);
	FastWrite(flash,command,4);
	Stop(flash);
	
	/*Writing into LFSR0TapEnable*/
	command[0] = LFSR0TapEnable | WRITE;
	command[1] = LFSR0TapEnableLength;
	command[2] = LFSR0TapEnableData_0;
	command[3] = LFSR0TapEnableData_1;
	Start(flash);
	FastWrite(flash,command,4);
	Stop(flash);
	
	/*Writing into TimingCtrl*/
	command[0] = TimingCtrl | WRITE;
	command[1] = TimingCtrlLength;
	command[2] = TimingCtrlData_0;
	Start(flash);
	FastWrite(flash,command,3);
	Stop(flash);
	
	/*Writing into SampleDelayCoarseTune*/
	command[0] = SampleDelayCoarseTune | WRITE;
	command[1] = SampleDelayCoarseTuneLength;
	command[2] = SampleDelayCoarseTuneData_0;
	command[3] = SampleDelayCoarseTuneData_1;
	Start(flash);
	FastWrite(flash,command,4);
	Stop(flash);
	
	/*Writing into SampleDelayMediumTune*/
	command[0] = SampleDelayMediumTune | WRITE;
	command[1] = SampleDelayMediumTuneLength;
	command[2] = SampleDelayMediumTuneData_0;
	Start(flash);
	FastWrite(flash,command,3);
	Stop(flash);
		
	/*Writing into SampleDelayFineTune*/
	command[0] = SampleDelayFineTune | WRITE;
	command[1] = SampleDelayFineTuneLength;
	command[2] = SampleDelayFineTuneData_0;
	Start(flash);
	FastWrite(flash,command,3);
	Stop(flash);
	
	/*Writing into SendPulseDelayCoarseTune*/
	command[0] = SendPulseDelayCoarseTune | WRITE;
	command[1] = SendPulseDelayCoarseTuneLength;
	command[2] = SendPulseDelayCoarseTuneData_0;
	command[3] = SendPulseDelayCoarseTuneData_1;
	Start(flash);
	FastWrite(flash,command,4);
	Stop(flash);

	/*Writing into SendPulseDelayMediumTune*/
	command[0] = SendPulseDelayMediumTune | WRITE;
	command[1] = SendPulseDelayMediumTuneLength;
	command[2] = SendPulseDelayMediumTuneData_0;
	Start(flash);
	FastWrite(flash,command,3);
	Stop(flash);
	
	/*Writing into SendPulseDelayFineTune*/
	command[0] = SendPulseDelayFineTune | WRITE;
	command[1] = SendPulseDelayFineTuneLength;
	command[2] = SendPulseDelayFineTuneData_0;
	Start(flash);
	FastWrite(flash,command,3);
	Stop(flash);


	/*Writing into TimingCalibrationCtrl*/
	command[0] = TimingCalibrationCtrl | WRITE;
	command[1] = TimingCalibrationCtrlLength;
	command[2] = TimingCalibrationCtrlData_0;
	Start(flash);
	FastWrite(flash,command,3);
	Stop(flash);

	/*Writing into MCLKOutputCtrl*/
	command[0] = MCLKOutputCtrl | WRITE;
	command[1] = MCLKOutputCtrlLength;
	command[2] = MCLKOutputCtrlData_0;
	Start(flash);
	FastWrite(flash,command,3);
	Stop(flash);

}

/*Checks if all the registers have been set to the pre-defined values*/
int checkInitializationRadar(struct mpsse_context * flash, char *command)
{
	unsigned char readbits1[24];
	int flag = 12; 
	command[0] = SampleReadOutCtrl | READ;
	command[1] = SampleReadOutCtrlLength;
	
	Start(flash);
	FastWrite(flash, command, 2);
	FastRead(flash, readbits1,2);
	Stop(flash);
	if(readbits1[0] != SampleReadOutCtrlData_0 || readbits1[1] != SampleReadOutCtrlData_1)
		{
		flag = 0;
		printf("Problem in SampleReadOutCtrl\n");
		}


	command[0] = SampleCtrl | READ;
	command[1] = SampleCtrlLength;	
	Start(flash);
	FastWrite(flash, command, 2);
	FastRead(flash, readbits1,1);
	Stop(flash);

	if(readbits1[0] != SampleCtrlData_0)
				{
		flag = 0;
		printf("Problem in SampleCtrl\n");
		}	

	command[0] = ThresholdPowerDown | READ;
	command[1] = ThresholdPowerDownLength; 	
	Start(flash);
	FastWrite(flash, command, 2);
	FastRead(flash, readbits1,1);
	Stop(flash);

	if(readbits1[0] != ThresholdPowerDownData_0)
		{
		flag = 0;
		printf("Problem in ThresholdPowerDown\n");
		}	
	
	command[0] = SampleInputCtrl | READ;
	command[1] = SampleInputCtrlLength;		
	Start(flash);
	FastWrite(flash, command, 2);
	FastRead(flash, readbits1,1);
	Stop(flash);

	if(readbits1[0] != SampleInputCtrlData_0)
		{
		flag = 0;
		printf("Problem in SampleInputCtrl\n");
		}

	command[0] = ThresholdCtrl | READ;
	command[1] = ThresholdCtrlLength;
	Start(flash);
	FastWrite(flash, command, 2);
	FastRead(flash, readbits1,1);
	Stop(flash);
	if(readbits1[0] != ThresholdCtrlData_0	)
		{
		flag = 0;
		printf("Problem in ThresholdCtrl\n");
		}	

	command[0] = NormalPulsesPerStep | READ;
	command[1] = NormalPulsesPerStepLength;
	Start(flash);
	FastWrite(flash, command, 2);
	FastRead(flash, readbits1,4);
	Stop(flash);

	if(readbits1[0] != NormalPulsesPerStepData_0 || readbits1[1] != NormalPulsesPerStepData_1 || 
readbits1[2] != NormalPulsesPerStepData_2 || readbits1[3] != NormalPulsesPerStepData_3)
		{
		flag = 0;
		printf("Problem in NormalPulsesPerStep\n");
		}		

	command[0] = FocusPulsesPerStep | READ;
	command[1] = FocusPulsesPerStepLength;
	Start(flash);
	FastWrite(flash, command, 2);
	FastRead(flash, readbits1,4);
	Stop(flash);

	if(readbits1[0] != FocusPulsesPerStepData_0 || readbits1[1] != FocusPulsesPerStepData_1 
|| readbits1[2] != FocusPulsesPerStepData_2 || readbits1[3] != FocusPulsesPerStepData_3)
		{
		flag = 0;
		printf("Problem in FocusPulsesPerStep\n");
		}

	command[0] = DACFirstIterationSetupTime;
	command[1] = DACFirstIterationSetupTimeLength;
	Start(flash);
	FastWrite(flash, command, 2);
	FastRead(flash, readbits1,2);
	Stop(flash);

	if(readbits1[0] != DACFirstIterationSetupTimeData_0 || readbits1[1] != DACFirstIterationSetupTimeData_1)
		{
		flag = 0;
		printf("Problem in DACFirstIterationSetupTime\n");
		}	

	command[0] = DACFirstStepSetupTime | READ;
	command[1] = DACFirstStepSetupTimeLength;	
	Start(flash);
	FastWrite(flash, command, 2);
	FastRead(flash, readbits1,2);
	Stop(flash);

	if(readbits1[0] != DACFirstStepSetupTimeData_0 || readbits1[1] != DACFirstStepSetupTimeData_1)
		{
		flag = 0;
		printf("Problem in DACFirstStepSetupTime\n");
		}	

	command[0] = DACRegularSetupTime | READ;
	command[1] = DACRegularSetupTimeLength;
	Start(flash);
	FastWrite(flash, command, 2);
	FastRead(flash, readbits1,2);
	Stop(flash);
	if(readbits1[0] != DACRegularSetupTimeData_0 || readbits1[1] != DACRegularSetupTimeData_1)
		{
		flag = 0;
		printf("Problem in DACRegularSetupTime\n");
		}		

	command[0] = DACLastIterationHoldTime | READ;
	command[1] = DACLastIterationHoldTimeLength;
	Start(flash);
	FastWrite(flash, command, 2);
	FastRead(flash, readbits1,2);
	Stop(flash);
	if(readbits1[0] != DACLastIterationHoldTimeData_0 || readbits1[1] != DACLastIterationHoldTimeData_1)
		{
		flag = 0;
		printf("Problem in DACLastIterationHoldTime\n");
		}		

	command[0] = DACLastStepHoldTime | READ;
	command[1] = DACLastStepHoldTimeLength;
	Start(flash);
	FastWrite(flash, command, 2);
	FastRead(flash, readbits1,2);
	Stop(flash);
	if(readbits1[0] != DACLastStepHoldTimeData_0 || readbits1[1] != DACLastStepHoldTimeData_1)
		{
		flag = 0;
		printf("Problem in DACLastStepHoldTime\n");
		}		

	command[0] = DACRegularHoldTime | READ;
	command[1] = DACRegularHoldTimeLength;
	Start(flash);
	FastWrite(flash, command, 2);
	FastRead(flash, readbits1,2);
	Stop(flash);
	if(readbits1[0] != DACRegularHoldTimeData_0 || readbits1[1] != DACRegularHoldTimeData_1)
		{
		flag = 0;
		printf("Problem in DACRegularHoldTime\n");
		}		

	command[0] = SweepMainCtrl | READ;
	command[1] = SweepMainCtrlLength;	
	Start(flash);
	FastWrite(flash, command, 2);
	FastRead(flash, readbits1,1);
	Stop(flash);
	if(readbits1[0] != SweepMainCtrlData_0)
		{
		flag = 0;
		printf("Problem in SweepMainCtrl\n");
		}


	command[0] = DACMax | READ;
	command[1] = DACMaxLength;
	Start(flash);
	FastWrite(flash, command, 2);
	FastRead(flash, readbits1,2);
	Stop(flash);
	if(readbits1[0] != DACMaxData_0 || readbits1[1] != DACMaxData_1)
		{
		flag = 0;
		printf("Problem in DACMax\n");
		}	

	command[0] = DACMin | READ;
	command[1] = DACMinLength;
	Start(flash);
	FastWrite(flash,command, 2);
	FastRead(flash, readbits1,2);
	Stop(flash);
	if(readbits1[0] != DACMinData_0 || readbits1[1] != DACMinData_1)
		{
		flag = 0;
		printf("Problem in DACMin\n");
		}	

	command[0] = DACstep | READ;
	command[1] = DACstepLength;
	Start(flash);
	FastWrite(flash, command , 2);
	FastRead(flash, readbits1,2);
	Stop(flash);
	
	if(readbits1[0] != DACstepData_0 || readbits1[1] !=DACstepData_1)
		{
		flag = 0;
		printf("Problem in DACstep\n");
		}	

	command[0] = Iterations | READ;
	command[1] = IterationsLength;	
	Start(flash);
	FastWrite(flash, command, 2);
	FastRead(flash, readbits1,2);
	Stop(flash);
	if(readbits1[0] != IterationsData_0 || readbits1[1] != IterationsData_1)
		{
		flag = 0;
		printf("Problem in Iterations\n");
		}

	command[0] = FocusMax | READ;
	command[1] = FocusMaxLength;
	Start(flash);
	FastWrite(flash, command, 2);
	FastRead(flash, readbits1,2);
	Stop(flash);
	
	if(readbits1[0] != FocusMaxData_0 || readbits1[1] != FocusMaxData_1)
		{
		flag = 0;
		printf("Problem in FocusMax\n");
		}	

	command[0] = FocusMin | READ;
	command[1] = FocusMinLength;
	Start(flash);
	FastWrite(flash, command, 2);
	FastRead(flash, readbits1,2);
	Stop(flash);
	if(readbits1[0] != FocusMinData_0 || readbits1[1] != FocusMinData_1)
		{
		flag = 0;
		printf("Problem in FocusMin\n");
		}	

	command[0] = FocusSetupTime | READ;
	command[1] = FocusSetupTimeLength;
	Start(flash);
	FastWrite(flash, command, 2);
	FastRead(flash, readbits1,1);
	Stop(flash);
	if(readbits1[0] != FocusSetupTimeData_0)
		{
		flag = 0;
		printf("Problem in FocusSetupTime\n");
		}	

	command[0] = FocusHoldTime | READ;
	command[1] = FocusHoldTimeLength; 
	Start(flash);
	FastWrite(flash, command , 2);
	FastRead(flash, readbits1,1);
	Stop(flash);
	if(readbits1[0] != FocusHoldTimeData_0)
		{
		flag = 0;
		printf("Problem in FocusHoldTime\n");
		}		

	command[0] = SweepClkCtrl | READ;
	command[1] = SweepClkCtrlLength;
	Start(flash);
	FastWrite(flash, command, 2);
	FastRead(flash, readbits1,1);
	Stop(flash);
	if(readbits1[0] != SweepClkCtrlData_0)
		{
		flag = 0;
		printf("Problem in SweepClkCtrl\n");
		}

	command[0] = PulseGeneratorCtrl | READ;
	command[1] = PulseGeneratorCtrlLength;
	Start(flash);
	FastWrite(flash, command, 2);
	FastRead(flash, readbits1,1);
	Stop(flash);
	
	if(readbits1[0] != PulseGeneratorCtrlData_0)
		{
		flag = 0;
		printf("Problem in PulseGeneratorCtrl\n");
		}	

	command[0]  =DACCtrl | READ;
	command[1] = DACCtrlLength;
	Start(flash);
	FastWrite(flash, command, 2);
	FastRead(flash, readbits1,2);
	Stop(flash);
	if(readbits1[0] != DACCtrlData_0 || readbits1[1] != DACCtrlData_1)
		{
		flag = 0;
		printf("Problem in DACCtrl\n");
		}	

	command[0] = MCLKCtrl | READ;
	command[1] = MCLKCtrlLength;
	Start(flash);
	FastWrite(flash, command, 2);
	FastRead(flash, readbits1,1);
	Stop(flash);
	if(readbits1[0] != MCLKCtrlData_0)
		{
		flag = 0;
		printf("Problem in MCLKCtrl\n");
		}
	command[0] = StaggeredPRFCtrl | READ;
	command[1] = StaggeredPRFCtrlLength;	
	Start(flash);
	FastWrite(flash, command, 2);
	FastRead(flash, readbits1,1);
	Stop(flash);
	if(readbits1[0] != StaggeredPRFCtrlData_0)
		{
		flag = 0;
		printf("Problem in StaggeredPRFCtrl\n");
		}	

	command[0] = StaggeredPRFDelay | READ;
	command[1] = StaggeredPRFDelayLength;
	Start(flash);
	FastWrite(flash, command, 2);
	FastRead(flash, readbits1,1);
	Stop(flash);
	if(readbits1[0] != StaggeredPRFDelayData_0)
		{
		flag = 0;
		printf("Problem in StaggeredPRFDelay\n");
		}
	
	command[0] = LFSR5TapEnable | READ;
	command[1] = LFSR5TapEnableLength;
	Start(flash);
	FastWrite(flash, command, 2);
	FastRead(flash, readbits1,2);
	Stop(flash);
	if(readbits1[0] != LFSR5TapEnableData_0 || readbits1[1] != LFSR5TapEnableData_1)
		{
		flag = 0;
		printf("Problem in LFSR5TapEnable\n");
		}		

	command[0] = LFSR4TapEnable | READ; 
	command[1] = LFSR4TapEnableLength;
	Start(flash);
	FastWrite(flash, command, 2);
	FastRead(flash, readbits1,2);
	Stop(flash);
	if(readbits1[0] != LFSR4TapEnableData_0 || readbits1[1] != LFSR4TapEnableData_1)
		{
		flag = 0;
		printf("Problem in LFSR4TapEnable\n");
		}

	command[0] = LFSR3TapEnable | READ;
	command[1] = LFSR3TapEnableLength;
	Start(flash);
	FastWrite(flash, command, 2);
	FastRead(flash, readbits1,2);
	Stop(flash);
	if(readbits1[0] != LFSR3TapEnableData_0 || readbits1[1] != LFSR3TapEnableData_1	)
		{
		flag = 0;
		printf("Problem in LFSR3TapEnable\n");
		}

	command[0] = LFSR2TapEnable | READ;
	command[1] = LFSR2TapEnableLength;
	Start(flash);
	FastWrite(flash, command, 2);
	FastRead(flash, readbits1,2);
	Stop(flash);
	if(readbits1[0] != LFSR2TapEnableData_0 || readbits1[1] != LFSR2TapEnableData_1)
		{
		flag = 0;
		printf("Problem in LFSR2TapEnable\n");
		}

	command[0] = LFSR1TapEnable | READ;
	command[1] = LFSR1TapEnableLength;
	Start(flash);
	FastWrite(flash, command, 2);
	FastRead(flash, readbits1,2);
	Stop(flash);
	if(readbits1[0] != LFSR1TapEnableData_0 || readbits1[1] != LFSR1TapEnableData_1)
		{
		flag = 0;
		printf("Problem in LFSR1TapEnable\n");
		}
	command[0] = LFSR0TapEnable | READ;
	command[1] = LFSR0TapEnableLength;
	Start(flash);
	FastWrite(flash, command, 2);
	FastRead(flash, readbits1,2);
	Stop(flash);
	if(readbits1[0] != LFSR0TapEnableData_0 || readbits1[1] != LFSR0TapEnableData_1)
		{
		flag = 0;
		printf("Problem in LFSR0TapEnable\n");
		}

	command[0] = TimingCtrl | READ;
	command[1] = TimingCtrlLength;
	Start(flash);
	FastWrite(flash, command, 2);
	FastRead(flash, readbits1,1);
	Stop(flash);
	if(readbits1[0] != TimingCtrlData_0)
		{
		flag = 0;
		printf("Problem in TimingCtrl\n");
		}		

	command[0] = SampleDelayCoarseTune | READ;
	command[1] = SampleDelayCoarseTuneLength;
	Start(flash);
	FastWrite(flash, command, 2);
	FastRead(flash, readbits1,2);
	Stop(flash);
	if(readbits1[0] != SampleDelayCoarseTuneData_0 || readbits1[1] != SampleDelayCoarseTuneData_1)
		{
		flag = 0;
		printf("Problem in SampleDelayCoarseTune\n");
		}
	command[0] = SampleDelayMediumTune | READ;
	command[1] = SampleDelayMediumTuneLength;
	Start(flash);
	FastWrite(flash, command, 2);
	FastRead(flash, readbits1,1);
	Stop(flash);
	if(readbits1[0] != SampleDelayMediumTuneData_0)
		{
		flag = 0;
		printf("Problem in SampleDelayMediumTune\n");
		}	

	command[0] = SampleDelayFineTune | READ;
	command[1] = SampleDelayFineTuneLength;	
	Start(flash);
	FastWrite(flash, command, 2);
	FastRead(flash, readbits1,1);
	Stop(flash);
	if(readbits1[0] != SampleDelayFineTuneData_0)
		{
		flag = 0;
		printf("Problem in SampleDelayFineTune\n");
		}	

	command[0] = SendPulseDelayCoarseTune | READ;
	command[1] = SendPulseDelayCoarseTuneLength;	
	Start(flash);
	FastWrite(flash, command, 2);
	FastRead(flash, readbits1,2);
	Stop(flash);
	if(readbits1[0] != SendPulseDelayCoarseTuneData_0 || readbits1[1] != SendPulseDelayCoarseTuneData_1)
		{
		flag = 0;
		printf("Problem in SendPulseDelayCoarseTune\n");
		}	

	command[0] = SendPulseDelayMediumTune | READ;
	command[1] = SendPulseDelayMediumTuneLength;
	Start(flash);
	FastWrite(flash, command, 2);
	FastRead(flash, readbits1,1);
	Stop(flash);
	if(readbits1[0] != SendPulseDelayMediumTuneData_0)
		{
		flag = 0;
		printf("Problem in SendPulseDelayMediumTune\n");
		}
	command[0] = SendPulseDelayFineTune | READ;
	command[1] = SendPulseDelayFineTuneLength;		
	Start(flash);
	FastWrite(flash, command, 2);
	FastRead(flash, readbits1,1);
	Stop(flash);
	if(readbits1[0] != SendPulseDelayFineTuneData_0)
		{
		flag = 0;
		printf("Problem in SendPulseDelayFineTune\n");
		}
	command[0] = TimingCalibrationCtrl | READ;
	command[1] = TimingCalibrationCtrlLength;	
	Start(flash);
	FastWrite(flash, command, 2);
	FastRead(flash, readbits1,1);
	Stop(flash);
	if(readbits1[0] != TimingCalibrationCtrlData_0)
		{
		flag = 0;
		printf("Problem in TimingCalibrationCtrl\n");
		}
	command[0] = MCLKOutputCtrl | READ;
	command[1] = MCLKOutputCtrlLength;	
	Start(flash);
	FastWrite(flash, command, 2);
	FastRead(flash, readbits1,1);
	Stop(flash);
	if(readbits1[0] != MCLKOutputCtrlData_0)
		{
		flag = 0;
		printf("Problem in MCLKOutputCtrl\n");
		}
	return flag;
}

/*Initializes and check's radar configurations. Returns 0 on error.*/
int initializeCheckRadar(struct mpsse_context * flash, char *command)
{
int flag =12;	
initializeRadar(flash, command);
flag = checkInitializationRadar(flash, command);
return flag;
}



/* Checks if key press occurs to safely exit infinite loops */
int kbhit()
{
    struct timeval tv;
    fd_set fds; 
    tv.tv_sec = 0;
    tv.tv_usec = 0;
    FD_ZERO(&fds);
    FD_SET(STDIN_FILENO, &fds); /*STDIN_FILENO is 0*/
    select(STDIN_FILENO+1, &fds, NULL, NULL, &tv);
    return FD_ISSET(STDIN_FILENO, &fds);
}

/* Clears the radar's counter and the output buffer.*/
void clearLastSweepLoadOutput(struct mpsse_context * flash)
{
	unsigned char readbits1[24];
	char command[10] ={0};

	/*Clears the result of last sweep*/
	command[0] = ResetCounters | WRITE;
	command[1] = DataLengthCommandStrobe;
	Start(flash);
	FastWrite(flash,command,2);	
	Stop(flash);

	/*Load Output buffer*/
	command[0] = LoadOutputBuffer | WRITE;
	command[1] = DataLengthCommandStrobe;	
	Start(flash);
	FastWrite(flash,command,2);
	Stop(flash);

}

void loadOuputBuffer(struct mpsse_context * flash)
{
	unsigned char readbits1[24];
	char command[10] ={0};

	/*Load Output buffer*/
	command[0] = LoadOutputBuffer | WRITE;
	command[1] = DataLengthCommandStrobe;	
	Start(flash);
	FastWrite(flash,command,2);
	Stop(flash);
}

/* Clears the previous sweep's results and starts a new sweep.*/
void startRadarSweep(struct mpsse_context * flash)
{
	unsigned char readbits1[24];
	char command[10] ={0};

	/*Clears the result of last swee*/
	command[0] = ResetCounters | WRITE;
	command[1] = DataLengthCommandStrobe;
	Start(flash);
	FastWrite(flash,command,2);	
	Stop(flash);
	/*Start sweep*/
	command[0] = StartSweep | WRITE;
	command[1] = DataLengthCommandStrobe;
	Start(flash);
	FastWrite(flash,command,2);
	Stop(flash);		
}

/* Waits until the sweep status bits becomes 0 indicating the completion of sweep. */
void monitorSweepStatus(struct mpsse_context * flash)
{
	unsigned char readbits1[24];
	char command[10] ={0};
	int i;		
	/*Monitor Status*/
	while(1)
	{
	/*printf("Wait\n");*/
	/* Checking Sweep status bits */
	command[0] = SweepControllerStatus | READ;
	command[1] = SweepControllerStatusLength;	
	Start(flash);
	FastWrite(flash,command,2);
	FastRead(flash, readbits1,2);
	Stop(flash);		
	if((readbits1[0] & 0x80)==0)
		{
		/*printf("Sampling over\n");*/
		break;
		}
 	
	}

}

/*Switching sequence is generated for the Mux control signal to select the required receiver antenna.
 Counter	- GPIO 3 	GPIO2	GPIO1	GPIO0 
	0	    0		0	0	0
	1	    0		0	0	1
	2	    0		0	1	0
	3	    0		0	1	1
	4	    0		1	0	0
	5	    0		1	0	1
	6	    0		1	1	0
	7	    0		1	1	1
	8	    1		0	0	0
	9	    1		0	0	1
	10	    1		0	1	0
	11	    1		0	1	1
	12	    1		1	0	0
	13	    1		1	0	1
	14	    1		1	1	0
	15	    1		1	1	1		    
*/
void switchingSequencer(struct mpsse_context * flash, int * counterRadarSelect)
{
	int idxSelect[16] = {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15};	
	
	*counterRadarSelect = rand() % 16;	
	switch(idxSelect[*counterRadarSelect])
	{	
		case 0: flash->pidle = (flash->pidle & (~GPIO0) & (~GPIO2) & (~GPIO3) & (~GPIO1));
			flash->pstart = (flash->pstart & (~GPIO0) & (~GPIO2) & (~GPIO3) & (~GPIO1));
			flash->pstop = (flash->pstop & (~GPIO0) & (~GPIO2) &(~GPIO3) &(~GPIO1));
			/*printf("%x is pidle\n,%x is pstart\n%x is pstop",(flash->pidle),flash->pstart,flash->pstop);*/
			break;
		case 1: 
			flash->pidle = (flash->pidle & (~GPIO2) & (~GPIO3) & (~GPIO1));
			flash->pidle = flash->pidle | (GPIO0) ;
			flash->pstart = (flash->pstart & (~GPIO2) & (~GPIO3) & (~GPIO1));
			flash->pstart = flash->pstart | (GPIO0) ;
			flash->pstop = (flash->pstop & (~GPIO2) & (~GPIO3) & (~GPIO1));
			flash->pstop = flash->pstop | (GPIO0) ;
			/*printf("%x is pidle\n,%x is pstart\n%x is pstop",(flash->pidle),flash->pstart,flash->pstop);*/
			break;
		case 2: 
			flash->pidle = (flash->pidle & (~GPIO0) & (~GPIO3) &(~GPIO2));
			flash->pidle = flash->pidle | (GPIO1); 
			flash->pstart = (flash->pstart & (~GPIO0) & (~GPIO3) & (~GPIO2));
			flash->pstart = flash->pstart | (GPIO1); 
			flash->pstop = (flash->pstop & (~GPIO0) & (~GPIO3) & (~GPIO2));
			flash->pstop = flash->pstop | (GPIO1); 
			/*printf("%x is pidle\n,%x is pstart\n%x is pstop",(flash->pidle),flash->pstart,flash->pstop);*/
			break;
		case 3: 
			flash->pidle = (flash->pidle  & (~GPIO3) & (~GPIO2));
			flash->pidle = (flash->pidle | (GPIO0) | (GPIO1));
			flash->pstart = (flash->pstart  & (~GPIO3) & (~GPIO2)) ;
			flash->pstart = (flash->pstart | (GPIO0) | (GPIO1));
			flash->pstop = (flash->pstop  & (~GPIO3) & (~GPIO2));
			flash->pstop = (flash->pstop | (GPIO0) | (GPIO1));
			/*printf("%x is pidle\n,%x is pstart\n%x is pstop",(flash->pidle),flash->pstart,flash->pstop);*/
			break;
		case 4: 
			flash->pidle = flash->pidle & (~GPIO0) & (~GPIO1) & (~GPIO3);
			flash->pidle = flash->pidle  | (GPIO2);		
			flash->pstart = flash->pstart & (~GPIO0) & (~GPIO1) & (~GPIO3);
			flash->pstart = flash->pstart  | (GPIO2);
			flash->pstop = flash->pstop & (~GPIO0) & (~GPIO1) & (~GPIO3);
			flash->pstop = flash->pstop  | (GPIO2);
			/*printf("%x is pidle\n,%x is pstart\n%x is pstop",(flash->pidle),flash->pstart,flash->pstop);*/
			break;
		case 5: 
			flash->pidle = flash->pidle & (~GPIO3) & (~GPIO1);
			flash->pidle = flash->pidle | (GPIO0) | (GPIO2);
			flash->pstart = flash->pstart & (~GPIO3) & (~GPIO1);
			flash->pstart = flash->pstart | (GPIO0) | (GPIO2);
			flash->pstop = flash->pstop & (~GPIO3) & (~GPIO1) ;
			flash->pstop = flash->pstop | (GPIO0) | (GPIO2);
			/*printf("%x is pidle\n,%x is pstart\n%x is pstop",(flash->pidle),flash->pstart,flash->pstop);*/
			break;
		case 6: 
			flash->pidle = flash->pidle & (~GPIO3) & (~GPIO0);
			flash->pidle = flash->pidle | (GPIO2) | (GPIO1) ; 
			flash->pstart = flash->pstart & (~GPIO3) & (~GPIO0);
			flash->pstart = flash->pstart | (GPIO2) | (GPIO1);
			flash->pstop = flash->pstop & (~GPIO3) & (~GPIO0);
			flash->pstop = flash->pstop | (GPIO2) | (GPIO1);
			/*printf("%x is pidle\n,%x is pstart\n%x is pstop",(flash->pidle),flash->pstart,flash->pstop);*/
			break;
		case 7: flash->pidle = flash->pidle & (~GPIO3);
			flash->pidle = (flash->pidle | (GPIO0) | (GPIO1) | (GPIO2));
			flash->pstart = flash->pstart & (~GPIO3);
			flash->pstart = (flash->pstart | (GPIO0) | (GPIO1) | (GPIO2));
			flash->pstop = flash->pstop & (~GPIO3) ;
			flash->pstop = (flash->pstop | (GPIO0) | (GPIO1) | (GPIO2));
			/*printf("%x is pidle\n,%x is pstart\n%x is pstop",(flash->pidle),flash->pstart,flash->pstop);*/
			break;	
		case 8: flash->pidle = (flash->pidle & (~GPIO0) & (~GPIO2)  & (~GPIO1));
			flash->pidle = flash->pidle  | (GPIO3);
			flash->pstart = (flash->pstart & (~GPIO0) & (~GPIO2) & (~GPIO1));
			flash->pstart = flash->pstart | (GPIO3);
			flash->pstop = (flash->pstop & (~GPIO0) & (~GPIO2) &(~GPIO1));
			flash->pstop = flash->pstop | (GPIO3);
			/*printf("%x is pidle\n,%x is pstart\n%x is pstop",(flash->pidle),flash->pstart,flash->pstop);*/
			break;
		case 9: 
			flash->pidle = (flash->pidle & (~GPIO2) & (~GPIO1));
			flash->pidle = flash->pidle | (GPIO0) | (GPIO3);
			flash->pstart = (flash->pstart & (~GPIO2) & (~GPIO1));
			flash->pstart = flash->pstart | (GPIO0) | (GPIO3);
			flash->pstop = (flash->pstop & (~GPIO2) & (~GPIO1));
			flash->pstop = flash->pstop | (GPIO0) | (GPIO3);
			/*printf("%x is pidle\n,%x is pstart\n%x is pstop",(flash->pidle),flash->pstart,flash->pstop);*/
			break;
		case 10: 
			flash->pidle = (flash->pidle & (~GPIO0)  &(~GPIO2));
			flash->pidle = flash->pidle | (GPIO1) | (GPIO3); 
			flash->pstart = (flash->pstart & (~GPIO0) & (~GPIO2));
			flash->pstart = flash->pstart | (GPIO1) | (GPIO3); 
			flash->pstop = (flash->pstop & (~GPIO0) & (~GPIO2));
			flash->pstop = flash->pstop | (GPIO1) | (GPIO3); 
			/*printf("%x is pidle\n,%x is pstart\n%x is pstop",(flash->pidle),flash->pstart,flash->pstop);*/
			break;
		case 11: 
			flash->pidle = (flash->pidle  & (~GPIO2));
			flash->pidle = (flash->pidle | (GPIO0) | (GPIO1) | (GPIO3));
			flash->pstart = (flash->pstart & (~GPIO2)) ;
			flash->pstart = (flash->pstart | (GPIO0) | (GPIO1) | (GPIO3));
			flash->pstop = (flash->pstop  & (~GPIO2));
			flash->pstop = (flash->pstop | (GPIO0) | (GPIO1) | (GPIO3));
			/*printf("%x is pidle\n,%x is pstart\n%x is pstop",(flash->pidle),flash->pstart,flash->pstop);*/
			break;
		case 12: 
			flash->pidle = flash->pidle & (~GPIO0) & (~GPIO1) ;
			flash->pidle = flash->pidle  | (GPIO2) | (GPIO3);		
			flash->pstart = flash->pstart & (~GPIO0) & (~GPIO1);
			flash->pstart = flash->pstart  | (GPIO2) | (GPIO3);
			flash->pstop = flash->pstop & (~GPIO0) & (~GPIO1);
			flash->pstop = flash->pstop  | (GPIO2) | (GPIO3);
			/*printf("%x is pidle\n,%x is pstart\n%x is pstop",(flash->pidle),flash->pstart,flash->pstop);*/
			break;
		case 13: 
			flash->pidle = flash->pidle & (~GPIO1);
			flash->pidle = flash->pidle | (GPIO0) | (GPIO2) | (GPIO3);
			flash->pstart = flash->pstart & (~GPIO1);
			flash->pstart = flash->pstart | (GPIO0) | (GPIO2) | (GPIO3);
			flash->pstop = flash->pstop & (~GPIO1) ;
			flash->pstop = flash->pstop | (GPIO0) | (GPIO2) | (GPIO3);
			/*printf("%x is pidle\n,%x is pstart\n%x is pstop",(flash->pidle),flash->pstart,flash->pstop);*/
			break;
		case 14: 
			flash->pidle = flash->pidle & (~GPIO0);
			flash->pidle = flash->pidle | (GPIO2) | (GPIO1) | (GPIO3) ; 
			flash->pstart = flash->pstart & (~GPIO0);
			flash->pstart = flash->pstart | (GPIO2) | (GPIO1) | (GPIO3);
			flash->pstop = flash->pstop  & (~GPIO0);
			flash->pstop = flash->pstop | (GPIO2) | (GPIO1) | (GPIO3);
			/*printf("%x is pidle\n,%x is pstart\n%x is pstop",(flash->pidle),flash->pstart,flash->pstop);*/
			break;
		case 15: 
			flash->pidle = (flash->pidle | (GPIO0) | (GPIO1) | (GPIO2) | (GPIO3));
			flash->pstart = (flash->pstart | (GPIO0) | (GPIO1) | (GPIO2) | (GPIO3));
			flash->pstop = (flash->pstop | (GPIO0) | (GPIO1) | (GPIO2) | (GPIO3));
			/*printf("%x is pidle\n,%x is pstart\n%x is pstop",(flash->pidle),flash->pstart,flash->pstop);*/
			break;	

	}
} 



/*Switching sequence is generated for the Mux control signal to select the required receiver antenna.
 Counter	- GPIO 3 	GPIO2	GPIO1	GPIO0 
	0	    0		0	0	0
	1	    0		0	0	1
	2	    0		0	1	0
	3	    0		0	1	1
	4	    0		1	0	0
	5	    0		1	0	1
	6	    0		1	1	0
	7	    0		1	1	1
	8	    1		0	0	0
	9	    1		0	0	1
	10	    1		0	1	0
	11	    1		0	1	1
	12	    1		1	0	0
	13	    1		1	0	1
	14	    1		1	1	0
	15	    1		1	1	1		    
*/
void switchingSequencerCalib(struct mpsse_context * flash, int * counterRadarSelect)
{
	int idxSelect[16] = {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15};	
	*counterRadarSelect = (*counterRadarSelect + 1) % 16;
	/* *counterRadarSelect = 1; */
	switch(idxSelect[*counterRadarSelect])
	{	
		case 0: flash->pidle = (flash->pidle & (~GPIO0) & (~GPIO2) & (~GPIO3) & (~GPIO1));
			flash->pstart = (flash->pstart & (~GPIO0) & (~GPIO2) & (~GPIO3) & (~GPIO1));
			flash->pstop = (flash->pstop & (~GPIO0) & (~GPIO2) &(~GPIO3) &(~GPIO1));
			/*printf("%x is pidle\n,%x is pstart\n%x is pstop",(flash->pidle),flash->pstart,flash->pstop);*/
			break;
		case 1: 
			flash->pidle = (flash->pidle & (~GPIO2) & (~GPIO3) & (~GPIO1));
			flash->pidle = flash->pidle | (GPIO0) ;
			flash->pstart = (flash->pstart & (~GPIO2) & (~GPIO3) & (~GPIO1));
			flash->pstart = flash->pstart | (GPIO0) ;
			flash->pstop = (flash->pstop & (~GPIO2) & (~GPIO3) & (~GPIO1));
			flash->pstop = flash->pstop | (GPIO0) ;
			/*printf("%x is pidle\n,%x is pstart\n%x is pstop",(flash->pidle),flash->pstart,flash->pstop);*/
			break;
		case 2: 
			flash->pidle = (flash->pidle & (~GPIO0) & (~GPIO3) &(~GPIO2));
			flash->pidle = flash->pidle | (GPIO1); 
			flash->pstart = (flash->pstart & (~GPIO0) & (~GPIO3) & (~GPIO2));
			flash->pstart = flash->pstart | (GPIO1); 
			flash->pstop = (flash->pstop & (~GPIO0) & (~GPIO3) & (~GPIO2));
			flash->pstop = flash->pstop | (GPIO1); 
			/*printf("%x is pidle\n,%x is pstart\n%x is pstop",(flash->pidle),flash->pstart,flash->pstop);*/
			break;
		case 3: 
			flash->pidle = (flash->pidle  & (~GPIO3) & (~GPIO2));
			flash->pidle = (flash->pidle | (GPIO0) | (GPIO1));
			flash->pstart = (flash->pstart  & (~GPIO3) & (~GPIO2)) ;
			flash->pstart = (flash->pstart | (GPIO0) | (GPIO1));
			flash->pstop = (flash->pstop  & (~GPIO3) & (~GPIO2));
			flash->pstop = (flash->pstop | (GPIO0) | (GPIO1));
			/*printf("%x is pidle\n,%x is pstart\n%x is pstop",(flash->pidle),flash->pstart,flash->pstop);*/
			break;
		case 4: 
			flash->pidle = flash->pidle & (~GPIO0) & (~GPIO1) & (~GPIO3);
			flash->pidle = flash->pidle  | (GPIO2);		
			flash->pstart = flash->pstart & (~GPIO0) & (~GPIO1) & (~GPIO3);
			flash->pstart = flash->pstart  | (GPIO2);
			flash->pstop = flash->pstop & (~GPIO0) & (~GPIO1) & (~GPIO3);
			flash->pstop = flash->pstop  | (GPIO2);
			/*printf("%x is pidle\n,%x is pstart\n%x is pstop",(flash->pidle),flash->pstart,flash->pstop);*/
			break;
		case 5: 
			flash->pidle = flash->pidle & (~GPIO3) & (~GPIO1);
			flash->pidle = flash->pidle | (GPIO0) | (GPIO2);
			flash->pstart = flash->pstart & (~GPIO3) & (~GPIO1);
			flash->pstart = flash->pstart | (GPIO0) | (GPIO2);
			flash->pstop = flash->pstop & (~GPIO3) & (~GPIO1) ;
			flash->pstop = flash->pstop | (GPIO0) | (GPIO2);
			/*printf("%x is pidle\n,%x is pstart\n%x is pstop",(flash->pidle),flash->pstart,flash->pstop);*/
			break;
		case 6: 
			flash->pidle = flash->pidle & (~GPIO3) & (~GPIO0);
			flash->pidle = flash->pidle | (GPIO2) | (GPIO1) ; 
			flash->pstart = flash->pstart & (~GPIO3) & (~GPIO0);
			flash->pstart = flash->pstart | (GPIO2) | (GPIO1);
			flash->pstop = flash->pstop & (~GPIO3) & (~GPIO0);
			flash->pstop = flash->pstop | (GPIO2) | (GPIO1);
			/*printf("%x is pidle\n,%x is pstart\n%x is pstop",(flash->pidle),flash->pstart,flash->pstop);*/
			break;
		case 7: flash->pidle = flash->pidle & (~GPIO3);
			flash->pidle = (flash->pidle | (GPIO0) | (GPIO1) | (GPIO2));
			flash->pstart = flash->pstart & (~GPIO3);
			flash->pstart = (flash->pstart | (GPIO0) | (GPIO1) | (GPIO2));
			flash->pstop = flash->pstop & (~GPIO3) ;
			flash->pstop = (flash->pstop | (GPIO0) | (GPIO1) | (GPIO2));
			/*printf("%x is pidle\n,%x is pstart\n%x is pstop",(flash->pidle),flash->pstart,flash->pstop);*/
			break;	
		case 8: flash->pidle = (flash->pidle & (~GPIO0) & (~GPIO2)  & (~GPIO1));
			flash->pidle = flash->pidle  | (GPIO3);
			flash->pstart = (flash->pstart & (~GPIO0) & (~GPIO2) & (~GPIO1));
			flash->pstart = flash->pstart | (GPIO3);
			flash->pstop = (flash->pstop & (~GPIO0) & (~GPIO2) &(~GPIO1));
			flash->pstop = flash->pstop | (GPIO3);
			/*printf("%x is pidle\n,%x is pstart\n%x is pstop",(flash->pidle),flash->pstart,flash->pstop);*/
			break;
		case 9: 
			flash->pidle = (flash->pidle & (~GPIO2) & (~GPIO1));
			flash->pidle = flash->pidle | (GPIO0) | (GPIO3);
			flash->pstart = (flash->pstart & (~GPIO2) & (~GPIO1));
			flash->pstart = flash->pstart | (GPIO0) | (GPIO3);
			flash->pstop = (flash->pstop & (~GPIO2) & (~GPIO1));
			flash->pstop = flash->pstop | (GPIO0) | (GPIO3);
			/*printf("%x is pidle\n,%x is pstart\n%x is pstop",(flash->pidle),flash->pstart,flash->pstop);*/
			break;
		case 10: 
			flash->pidle = (flash->pidle & (~GPIO0)  &(~GPIO2));
			flash->pidle = flash->pidle | (GPIO1) | (GPIO3); 
			flash->pstart = (flash->pstart & (~GPIO0) & (~GPIO2));
			flash->pstart = flash->pstart | (GPIO1) | (GPIO3); 
			flash->pstop = (flash->pstop & (~GPIO0) & (~GPIO2));
			flash->pstop = flash->pstop | (GPIO1) | (GPIO3); 
			/*printf("%x is pidle\n,%x is pstart\n%x is pstop",(flash->pidle),flash->pstart,flash->pstop);*/
			break;
		case 11: 
			flash->pidle = (flash->pidle  & (~GPIO2));
			flash->pidle = (flash->pidle | (GPIO0) | (GPIO1) | (GPIO3));
			flash->pstart = (flash->pstart & (~GPIO2)) ;
			flash->pstart = (flash->pstart | (GPIO0) | (GPIO1) | (GPIO3));
			flash->pstop = (flash->pstop  & (~GPIO2));
			flash->pstop = (flash->pstop | (GPIO0) | (GPIO1) | (GPIO3));
			/*printf("%x is pidle\n,%x is pstart\n%x is pstop",(flash->pidle),flash->pstart,flash->pstop);*/
			break;
		case 12: 
			flash->pidle = flash->pidle & (~GPIO0) & (~GPIO1) ;
			flash->pidle = flash->pidle  | (GPIO2) | (GPIO3);		
			flash->pstart = flash->pstart & (~GPIO0) & (~GPIO1);
			flash->pstart = flash->pstart  | (GPIO2) | (GPIO3);
			flash->pstop = flash->pstop & (~GPIO0) & (~GPIO1);
			flash->pstop = flash->pstop  | (GPIO2) | (GPIO3);
			/*printf("%x is pidle\n,%x is pstart\n%x is pstop",(flash->pidle),flash->pstart,flash->pstop);*/
			break;
		case 13: 
			flash->pidle = flash->pidle & (~GPIO1);
			flash->pidle = flash->pidle | (GPIO0) | (GPIO2) | (GPIO3);
			flash->pstart = flash->pstart & (~GPIO1);
			flash->pstart = flash->pstart | (GPIO0) | (GPIO2) | (GPIO3);
			flash->pstop = flash->pstop & (~GPIO1) ;
			flash->pstop = flash->pstop | (GPIO0) | (GPIO2) | (GPIO3);
			/*printf("%x is pidle\n,%x is pstart\n%x is pstop",(flash->pidle),flash->pstart,flash->pstop);*/
			break;
		case 14: 
			flash->pidle = flash->pidle & (~GPIO0);
			flash->pidle = flash->pidle | (GPIO2) | (GPIO1) | (GPIO3) ; 
			flash->pstart = flash->pstart & (~GPIO0);
			flash->pstart = flash->pstart | (GPIO2) | (GPIO1) | (GPIO3);
			flash->pstop = flash->pstop  & (~GPIO0);
			flash->pstop = flash->pstop | (GPIO2) | (GPIO1) | (GPIO3);
			/*printf("%x is pidle\n,%x is pstart\n%x is pstop",(flash->pidle),flash->pstart,flash->pstop);*/
			break;
		case 15: 
			flash->pidle = (flash->pidle | (GPIO0) | (GPIO1) | (GPIO2) | (GPIO3));
			flash->pstart = (flash->pstart | (GPIO0) | (GPIO1) | (GPIO2) | (GPIO3));
			flash->pstop = (flash->pstop | (GPIO0) | (GPIO1) | (GPIO2) | (GPIO3));
			/*printf("%x is pidle\n,%x is pstart\n%x is pstop",(flash->pidle),flash->pstart,flash->pstop);*/
			break;	

	}
} 
void switchingSequencerModified(struct mpsse_context * flash, int radarSelect)
{
	int idxSelect[16] = {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15};	

	switch(radarSelect)
	{	
		case 0: flash->pidle = (flash->pidle & (~GPIO0) & (~GPIO2) & (~GPIO3) & (~GPIO1));
			flash->pstart = (flash->pstart & (~GPIO0) & (~GPIO2) & (~GPIO3) & (~GPIO1));
			flash->pstop = (flash->pstop & (~GPIO0) & (~GPIO2) &(~GPIO3) &(~GPIO1));
			/*printf("%x is pidle\n,%x is pstart\n%x is pstop",(flash->pidle),flash->pstart,flash->pstop);*/
			break;
		case 1: 
			flash->pidle = (flash->pidle & (~GPIO2) & (~GPIO3) & (~GPIO1));
			flash->pidle = flash->pidle | (GPIO0) ;
			flash->pstart = (flash->pstart & (~GPIO2) & (~GPIO3) & (~GPIO1));
			flash->pstart = flash->pstart | (GPIO0) ;
			flash->pstop = (flash->pstop & (~GPIO2) & (~GPIO3) & (~GPIO1));
			flash->pstop = flash->pstop | (GPIO0) ;
			/*printf("%x is pidle\n,%x is pstart\n%x is pstop",(flash->pidle),flash->pstart,flash->pstop);*/
			break;
		case 2: 
			flash->pidle = (flash->pidle & (~GPIO0) & (~GPIO3) &(~GPIO2));
			flash->pidle = flash->pidle | (GPIO1); 
			flash->pstart = (flash->pstart & (~GPIO0) & (~GPIO3) & (~GPIO2));
			flash->pstart = flash->pstart | (GPIO1); 
			flash->pstop = (flash->pstop & (~GPIO0) & (~GPIO3) & (~GPIO2));
			flash->pstop = flash->pstop | (GPIO1); 
			/*printf("%x is pidle\n,%x is pstart\n%x is pstop",(flash->pidle),flash->pstart,flash->pstop);*/
			break;
		case 3: 
			flash->pidle = (flash->pidle  & (~GPIO3) & (~GPIO2));
			flash->pidle = (flash->pidle | (GPIO0) | (GPIO1));
			flash->pstart = (flash->pstart  & (~GPIO3) & (~GPIO2)) ;
			flash->pstart = (flash->pstart | (GPIO0) | (GPIO1));
			flash->pstop = (flash->pstop  & (~GPIO3) & (~GPIO2));
			flash->pstop = (flash->pstop | (GPIO0) | (GPIO1));
			/*printf("%x is pidle\n,%x is pstart\n%x is pstop",(flash->pidle),flash->pstart,flash->pstop);*/
			break;
		case 4: 
			flash->pidle = flash->pidle & (~GPIO0) & (~GPIO1) & (~GPIO3);
			flash->pidle = flash->pidle  | (GPIO2);		
			flash->pstart = flash->pstart & (~GPIO0) & (~GPIO1) & (~GPIO3);
			flash->pstart = flash->pstart  | (GPIO2);
			flash->pstop = flash->pstop & (~GPIO0) & (~GPIO1) & (~GPIO3);
			flash->pstop = flash->pstop  | (GPIO2);
			/*printf("%x is pidle\n,%x is pstart\n%x is pstop",(flash->pidle),flash->pstart,flash->pstop);*/
			break;
		case 5: 
			flash->pidle = flash->pidle & (~GPIO3) & (~GPIO1);
			flash->pidle = flash->pidle | (GPIO0) | (GPIO2);
			flash->pstart = flash->pstart & (~GPIO3) & (~GPIO1);
			flash->pstart = flash->pstart | (GPIO0) | (GPIO2);
			flash->pstop = flash->pstop & (~GPIO3) & (~GPIO1) ;
			flash->pstop = flash->pstop | (GPIO0) | (GPIO2);
			/*printf("%x is pidle\n,%x is pstart\n%x is pstop",(flash->pidle),flash->pstart,flash->pstop);*/
			break;
		case 6: 
			flash->pidle = flash->pidle & (~GPIO3) & (~GPIO0);
			flash->pidle = flash->pidle | (GPIO2) | (GPIO1) ; 
			flash->pstart = flash->pstart & (~GPIO3) & (~GPIO0);
			flash->pstart = flash->pstart | (GPIO2) | (GPIO1);
			flash->pstop = flash->pstop & (~GPIO3) & (~GPIO0);
			flash->pstop = flash->pstop | (GPIO2) | (GPIO1);
			/*printf("%x is pidle\n,%x is pstart\n%x is pstop",(flash->pidle),flash->pstart,flash->pstop);*/
			break;
		case 7: flash->pidle = flash->pidle & (~GPIO3);
			flash->pidle = (flash->pidle | (GPIO0) | (GPIO1) | (GPIO2));
			flash->pstart = flash->pstart & (~GPIO3);
			flash->pstart = (flash->pstart | (GPIO0) | (GPIO1) | (GPIO2));
			flash->pstop = flash->pstop & (~GPIO3) ;
			flash->pstop = (flash->pstop | (GPIO0) | (GPIO1) | (GPIO2));
			/*printf("%x is pidle\n,%x is pstart\n%x is pstop",(flash->pidle),flash->pstart,flash->pstop);*/
			break;	
		case 8: flash->pidle = (flash->pidle & (~GPIO0) & (~GPIO2)  & (~GPIO1));
			flash->pidle = flash->pidle  | (GPIO3);
			flash->pstart = (flash->pstart & (~GPIO0) & (~GPIO2) & (~GPIO1));
			flash->pstart = flash->pstart | (GPIO3);
			flash->pstop = (flash->pstop & (~GPIO0) & (~GPIO2) &(~GPIO1));
			flash->pstop = flash->pstop | (GPIO3);
			/*printf("%x is pidle\n,%x is pstart\n%x is pstop",(flash->pidle),flash->pstart,flash->pstop);*/
			break;
		case 9: 
			flash->pidle = (flash->pidle & (~GPIO2) & (~GPIO1));
			flash->pidle = flash->pidle | (GPIO0) | (GPIO3);
			flash->pstart = (flash->pstart & (~GPIO2) & (~GPIO1));
			flash->pstart = flash->pstart | (GPIO0) | (GPIO3);
			flash->pstop = (flash->pstop & (~GPIO2) & (~GPIO1));
			flash->pstop = flash->pstop | (GPIO0) | (GPIO3);
			/*printf("%x is pidle\n,%x is pstart\n%x is pstop",(flash->pidle),flash->pstart,flash->pstop);*/
			break;
		case 10: 
			flash->pidle = (flash->pidle & (~GPIO0)  &(~GPIO2));
			flash->pidle = flash->pidle | (GPIO1) | (GPIO3); 
			flash->pstart = (flash->pstart & (~GPIO0) & (~GPIO2));
			flash->pstart = flash->pstart | (GPIO1) | (GPIO3); 
			flash->pstop = (flash->pstop & (~GPIO0) & (~GPIO2));
			flash->pstop = flash->pstop | (GPIO1) | (GPIO3); 
			/*printf("%x is pidle\n,%x is pstart\n%x is pstop",(flash->pidle),flash->pstart,flash->pstop);*/
			break;
		case 11: 
			flash->pidle = (flash->pidle  & (~GPIO2));
			flash->pidle = (flash->pidle | (GPIO0) | (GPIO1) | (GPIO3));
			flash->pstart = (flash->pstart & (~GPIO2)) ;
			flash->pstart = (flash->pstart | (GPIO0) | (GPIO1) | (GPIO3));
			flash->pstop = (flash->pstop  & (~GPIO2));
			flash->pstop = (flash->pstop | (GPIO0) | (GPIO1) | (GPIO3));
			/*printf("%x is pidle\n,%x is pstart\n%x is pstop",(flash->pidle),flash->pstart,flash->pstop);*/
			break;
		case 12: 
			flash->pidle = flash->pidle & (~GPIO0) & (~GPIO1) ;
			flash->pidle = flash->pidle  | (GPIO2) | (GPIO3);		
			flash->pstart = flash->pstart & (~GPIO0) & (~GPIO1);
			flash->pstart = flash->pstart  | (GPIO2) | (GPIO3);
			flash->pstop = flash->pstop & (~GPIO0) & (~GPIO1);
			flash->pstop = flash->pstop  | (GPIO2) | (GPIO3);
			/*printf("%x is pidle\n,%x is pstart\n%x is pstop",(flash->pidle),flash->pstart,flash->pstop);*/
			break;
		case 13: 
			flash->pidle = flash->pidle & (~GPIO1);
			flash->pidle = flash->pidle | (GPIO0) | (GPIO2) | (GPIO3);
			flash->pstart = flash->pstart & (~GPIO1);
			flash->pstart = flash->pstart | (GPIO0) | (GPIO2) | (GPIO3);
			flash->pstop = flash->pstop & (~GPIO1) ;
			flash->pstop = flash->pstop | (GPIO0) | (GPIO2) | (GPIO3);
			/*printf("%x is pidle\n,%x is pstart\n%x is pstop",(flash->pidle),flash->pstart,flash->pstop);*/
			break;
		case 14: 
			flash->pidle = flash->pidle & (~GPIO0);
			flash->pidle = flash->pidle | (GPIO2) | (GPIO1) | (GPIO3) ; 
			flash->pstart = flash->pstart & (~GPIO0);
			flash->pstart = flash->pstart | (GPIO2) | (GPIO1) | (GPIO3);
			flash->pstop = flash->pstop  & (~GPIO0);
			flash->pstop = flash->pstop | (GPIO2) | (GPIO1) | (GPIO3);
			/*printf("%x is pidle\n,%x is pstart\n%x is pstop",(flash->pidle),flash->pstart,flash->pstop);*/
			break;
		case 15: 
			flash->pidle = (flash->pidle | (GPIO0) | (GPIO1) | (GPIO2) | (GPIO3));
			flash->pstart = (flash->pstart | (GPIO0) | (GPIO1) | (GPIO2) | (GPIO3));
			flash->pstop = (flash->pstop | (GPIO0) | (GPIO1) | (GPIO2) | (GPIO3));
			/*printf("%x is pidle\n,%x is pstart\n%x is pstop",(flash->pidle),flash->pstart,flash->pstop);*/
			break;	


	}
	
}

/* Retrieves the data from the output buffer using our modified fast write and fast read to reduce the number of calls to 
 libusb_bulk_transfer in the libusb library.*/

void periodicFunc(struct mpsse_context * flash, unsigned char *dataRead)
{
	unsigned char readbits1[24];
	char command[50] ={0},command1[50] ={0};
	int txSize[20]={0},rxSize[20]={0};
	struct timeval t1,t2;	
	
	int i,j;
		
	command1[0] = SampleOutputBuffer | READ;
	command1[1] = SampleOutputBufferStartLength;
	command1[2] = SampleOutputBuffer | READ;
	command1[3] = SampleOutputBufferContinueLength;
	command1[4] = SampleOutputBuffer | READ;
	command1[5] = SampleOutputBufferContinueLength;
	command1[6] = SampleOutputBuffer | READ;
	command1[7] = SampleOutputBufferContinueLength;
	command1[8] = SampleOutputBuffer | READ;
	command1[9] = SampleOutputBufferContinueLength;
	command1[10] = SampleOutputBuffer | READ;
	command1[11] = SampleOutputBufferContinueLength;
	command1[12] = SampleOutputBuffer | READ;
	command1[13] = SampleOutputBufferContinueLength;
	command1[14] = SampleOutputBuffer | READ;
	command1[15] = SampleOutputBufferContinueLength;
	command1[16] = SampleOutputBuffer | READ;
	command1[17] = SampleOutputBufferContinueLength;
	command1[18] = SampleOutputBuffer | READ;
	command1[19] = SampleOutputBufferContinueLength;
	command1[20] = SampleOutputBuffer | READ;
	command1[21] = SampleOutputBufferContinueLength;
	command1[22] = SampleOutputBuffer | READ;
	command1[23] = SampleOutputBufferContinueLength;
	command1[24] = SampleOutputBuffer | READ;
	command1[25] = SampleOutputBufferContinueLength;
	command1[26] = SampleOutputBuffer | READ;
	command1[27] = SampleOutputBufferContinueLength;
	command1[28] = SampleOutputBuffer | READ;
	command1[29] = SampleOutputBufferContinueLength;
	command1[30] = SampleOutputBuffer | READ;
	command1[31] = SampleOutputBufferContinueLength;
	command1[32] = SampleOutputBuffer | READ;
	command1[33] = SampleOutputBufferFinalLength;
	txSize[0] = 2;
	txSize[1] = 2;
	txSize[2] = 2;
	txSize[3] = 2;
	txSize[4] = 2;
	txSize[5] = 2;
	txSize[6] = 2;
	txSize[7] = 2;
	txSize[8] = 2;
	txSize[9] = 2;
	txSize[10] = 2;
	txSize[11] = 2;
	txSize[12] = 2;
	txSize[13] = 2;
	txSize[14] = 2;
	txSize[15] = 2;
	txSize[16] = 2;

	rxSize[0] = 127;
	rxSize[1] = 127;
	rxSize[2] = 127;
	rxSize[3] = 127;
	rxSize[4] = 127;
	rxSize[5] = 127;
	rxSize[6] = 127;
	rxSize[7] = 127;
	rxSize[8] = 127;
	rxSize[9] = 127;
	rxSize[10] = 127;
	rxSize[11] = 127;
	rxSize[12] = 127;
	rxSize[13] = 127;
	rxSize[14] = 127;
	rxSize[15] = 127;
	rxSize[16] = 16;
	if(Start(flash) != MPSSE_OK) printf("problem at 1121\n");

	if(FastWriteMod(flash,command1,txSize,17,rxSize) != MPSSE_OK) printf("problem at 1124\n");
	if(FastReady(flash, dataRead, 128*16)!=MPSSE_OK) printf("problem at 1142\n");
	Stop(flash);

/*		
	for(i=0;i<2;i++)
		{	
		for(j=0;j<16;j++)		
			command[j] = command1[j+i*16];
		
		txSize[0] = 2;
		txSize[1] = 2;
		txSize[2] = 2;
		txSize[3] = 2;
		txSize[4] = 2;
		txSize[5] = 2;
		txSize[6] = 2;
		txSize[7] = 2;


		rxSize[0] = 127;
		rxSize[1] = 127;
		rxSize[2] = 127;
		rxSize[3] = 127;
		rxSize[4] = 127;
		rxSize[5] = 127;
		rxSize[6] = 127;
		rxSize[7] = 127;
		

		struct timespec req;
		req.tv_sec=0;
		req.tv_nsec = 100000;	
			
		if(Start(flash) != MPSSE_OK) printf("problem at 1121\n");

		if(FastWriteMod(flash,command,txSize,8,rxSize) != MPSSE_OK) printf("problem at 1124\n");
		if(FastReady(flash, dataRead + (i*127*8), 127*8)!=MPSSE_OK) printf("problem at 1142\n");
	
		
		Stop(flash);
		}
		command[0] = SampleOutputBuffer | READ;
		command[1] = SampleOutputBufferFinalLength;
		Start(flash);
		FastWrite(flash,command,2);
		FastRead(flash, dataRead+127*16,16);
		Stop(flash);	*/
}

void processFile(char *fileName, char *fileNameFinal)
{
	long lSize;
	unsigned char *buffer;
	double *bufferFloat;  	
	uint32_t *buffer1;
	long int counter=0;
	int i;
	FILE *fp = NULL;
	fp = fopen(fileName,"rb");
  	/* obtain file size */
  	fseek (fp , 0 , SEEK_END);
  	lSize = ftell (fp);
 	rewind (fp);

  	/* allocate memory to contain the whole file*/
  	buffer = (unsigned char*) malloc (sizeof(unsigned char)*lSize);
  	buffer1 = (uint32_t*) malloc (sizeof(uint32_t)*(lSize/4));
	bufferFloat = (double*) malloc(sizeof(double)*(lSize/4));
  	if (buffer == NULL) {fputs ("Memory error",stderr); exit (2);}
	
  	/* copy the file into the buffer*/
   	fread (buffer,1,lSize,fp);	
	fclose(fp);

	/*for(i=0;i<2040;i+=8)
		printf("%x\t%x\t%x\t%x::\t%x\t%x\t%x\t%x\n",buffer[i],buffer[i+1],buffer[i+2],buffer[i+3],buffer[i+4],buffer[i+5],buffer[i+6],buffer[i+7]);*/
	fp = fopen(fileNameFinal,"w");

	for(i=0;i<lSize-3;i=i+4)
	{
		buffer1[counter] = (buffer[i] << 24) | (buffer[i+1] << 16) | (buffer[i+2] << 8) | buffer[i+3];
		bufferFloat[counter] = (double)(buffer1[counter]/(iterationsVal*avgFactorVal) + dacMinVal ) *dacStepVal/81.91;

		fprintf(fp,"%f\n",bufferFloat[counter]);	
		counter=counter+1;
	}
	fclose(fp);
	printf("\n %ld is the number of samples\n",counter);
}

void processData(unsigned char *buffer, float *buffer1)
{
	int i,counter=0;

	for(i=0;i<2045;i=i+4)
	{
		buffer1[counter] = (double)(( (buffer[i] << 24) | (buffer[i+1] << 16) | (buffer[i+2] << 8) | buffer[i+3])/1.0);	


buffer1[counter] = (buffer1[counter]/(iterationsVal*avgFactorVal) + dacMinVal ) *dacStepVal/81.91;

		counter=counter+1;
	}
}

void processDataAvg(unsigned char *buffer, float *buffer1, int counterFrames)
{
	int i,counter=0;
	float tempVal=0;
	
	for(i=0;i<2045;i=i+4)
	{
		tempVal =  (float)(( (buffer[i] << 24) | (buffer[i+1] << 16) | (buffer[i+2] << 8) | buffer[i+3])/1.0);	

tempVal = (tempVal/(iterationsVal*avgFactorVal) + dacMinVal ) *dacStepVal/81.91;buffer1[counter] = buffer1[counter]*(counterFrames-1)/counterFrames + tempVal/counterFrames;

		counter=counter+1;
	}
	
}
int readConfigFile()
{
	FILE *fp = NULL;
	
	long lSize;
	int i;
	unsigned char *buffer;  
	fp = fopen(RADAR_CONFIG,"rb");
  	if(fp == NULL) 
	{
		printf("Unable to read configuration file\n");
		return ERROR_CONFIG_FILE_MISSING;
	}

	/* obtain file size*/
  	fseek (fp , 0 , SEEK_END);
  	lSize = ftell (fp);
 	rewind (fp);
	/* allocate memory to contain the whole file*/
  	buffer = (unsigned char*) malloc (sizeof(unsigned char)*lSize);
	/* copy the file into the buffer*/
   	fread (buffer,1,lSize,fp);	
	fclose(fp);

	SampleReadOutCtrlData_1 = SampleReadOutCtrlData_1 | buffer[0]; 
	ThresholdCtrlData_0 = ThresholdCtrlData_0 | buffer[1];
	FocusPulsesPerStepData_0 = FocusPulsesPerStepData_0 | buffer[2];
	FocusPulsesPerStepData_1 = FocusPulsesPerStepData_1 | buffer[3];
	FocusPulsesPerStepData_2 = FocusPulsesPerStepData_2 | buffer[4];
	FocusPulsesPerStepData_3 = FocusPulsesPerStepData_3 | buffer[5];
	NormalPulsesPerStepData_0 = NormalPulsesPerStepData_0 | buffer[2];
	NormalPulsesPerStepData_1 = NormalPulsesPerStepData_1 | buffer[3];
	NormalPulsesPerStepData_2 = NormalPulsesPerStepData_2 | buffer[4];
	NormalPulsesPerStepData_3 = NormalPulsesPerStepData_3 | buffer[5];
	DACMaxData_0 = DACMaxData_0 | buffer[6];
	DACMaxData_1 = DACMaxData_1 | buffer[7];
	FocusMaxData_0 = FocusMaxData_0 | buffer[6];
	FocusMaxData_1 = FocusMaxData_1 | buffer[7];
	DACMinData_0 = DACMinData_0 | buffer[8];
	DACMinData_1 = DACMinData_1 | buffer[9];
	FocusMinData_0 = FocusMinData_0 | buffer[8];
	FocusMinData_1 = FocusMinData_1 | buffer[9];
	DACstepData_0 = DACstepData_0 | buffer[10];
	DACstepData_1 = DACstepData_1 | buffer[11];
	IterationsData_0 = IterationsData_0 | buffer[12];
	IterationsData_1 = IterationsData_1 | buffer[13];
	SampleDelayCoarseTuneData_0 = SampleDelayCoarseTuneData_0 | buffer[14];
	SampleDelayCoarseTuneData_1 = SampleDelayCoarseTuneData_1 | buffer[15];
	SampleDelayMediumTuneData_0 = SampleDelayMediumTuneData_0 | buffer[16];
	frameRate = buffer[17];
	
	

	iterationsVal = ((IterationsData_0 << 8) + IterationsData_1)/1.0; 
	avgFactorVal = ((NormalPulsesPerStepData_0 << 24 ) + (NormalPulsesPerStepData_1 << 16)  +  (NormalPulsesPerStepData_2 << 8)  + (NormalPulsesPerStepData_3) )/1.0;
	dacMinVal = ((DACMinData_0 << 8) + DACMinData_1)/1.0; 
	dacStepVal = ((DACstepData_0 << 8) + DACstepData_1)/1.0; 

return SUCCESS_1;

}

void swap(int *a,int i, int j )
{
	int temp;
	temp = *(a+i);
	*(a+i) = *(a+j);
	*(a+j) = temp;
}

void shuffle(int *a, int sizeArray )
{
	int i,j;	

	for(i=sizeArray-1;i >=1 ;i--)
	{
		j = rand()%(i+1);
		swap(a,i,j);
	}
		

}

