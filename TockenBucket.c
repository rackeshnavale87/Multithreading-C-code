/*
 * Author:      Rakesh Sharad Navale.
 * @(#)$Id: TockenBucket.c,v 1.1 2015/01/15 20:15:30 $
 */
#include<math.h>
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<sys/time.h>
#include<pthread.h>
#include<unistd.h>
#include<signal.h>
#include<fcntl.h>
#include<errno.h>
#include"my402list.h" // private header file
#include"TockenBucket.h" // private header file
/*	 _________________________________________________________________________
        |(mutex lock)								  |
	|			         _______ 'r' rate incoming tockens	  |
	|			         |  t  | \ 				  |
	|			         |  t  |  \				  |
	|			         |  t  |   }> 'B'			  |
	|			         |  t  |  /				  |
	|			         |  t  | / 				  |
	|			         =======				  |
   	|				    |			            ~----------> Server1 \
	|  incoming 'n' packets		    |			           /	  |	          } = at 'mu' rate
     ---|------> Q1[][]...[][][] -------->--*--------> Q2[][][]...[][][]---------------> Server2 /
	| 'lambda' rate								  |          
	| 									  |
	|_________________________________________________________________________|
		


* Function parses input arguments supplied from commandline, to get values of all the options and check for errors
* The command line syntax for TockenBucket is as follows : TockenBucket [-lambda lambda] [-mu mu] [-r r] [-B B] [-P P] [-n num] [-t tsfile]
* A commandline option is a commandline argument that begins with a - character in a commandline syntax specification
* The -n option specifies the total number of packets to arrive
* If the -t option is specified, tsfile is a trace specification file that you should use to drive your emulation
* In this case, you should ignore the -lambda, -mu, -P, and -num commandline options and run your emulation in the trace-driven mode
* If the -t option is not used, you should run your emulation in the deterministic mode
* The default value (i.e., if it's not specified in a commandline option) for lambda is 1 (packets per second), the default value for mu is 0.35
  (packets per second), the default value for r is 1.5 (tokens per second), the default value for B is 10 (tokens), the default value for P is 3 
  (tokens), and the default value for num is 20 (packets). B, P, and num must be positive integers with a maximum value of 2147483647 (0x7fffffff). 
  lambda, mu, and r must be positive real numbers
*/

struct timeval emulation_Start_Time;
struct timeval emulation_End_Time;

pthread_t packetMonitorThread;
pthread_t tokenMonitorThread;
pthread_t serverMonitorThread1;
pthread_t serverMonitorThread2;
pthread_t serverMonitorThread3;
pthread_t serverMonitorThread4;
pthread_t serverMonitorThread5;
pthread_t signalMonitorThread;
pthread_mutex_t sync_Mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t out = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t sync_CV_Q2Packet = PTHREAD_COND_INITIALIZER;
sigset_t intrpt_Signal;


int TS=5;
char *t = NULL;
float lambda = 1.0, mu = 0.35, r = 1.5;
int B = 10, P = 3, n = 20, finish_P_ArrivalThread = 0, finish_TokenThread = 0, finish_ServerThread = 0, signal_break_Flag = 0;
int tFile_check = 0, everyToken_Counter = 0, tokenBucket_Counter = 0, droppedToken_Counter = 0, mainPacket_Counter = 0, dropPacket_Counter = 0, removedPacket_Counter = 0;
double totalService_Time = 0,totalService_Time_S1 = 0,totalService_Time_S2 = 0,totalService_Time_S3=0, totalService_Time_S4=0,totalService_Time_S5=0,totalInterarrival_Time = 0, totalTime_In_Q1 = 0, totalTime_In_Q2 = 0, totalEmulation_RunTime;
long double totalTime_In_System_Sqrd = 0, totalTime_In_System = 0;
int number_S1_delivered=0,number_S2_delivered=0,number_S3_delivered=0,number_S4_delivered=0,number_S5_delivered=0;

My402List *packet_Arrival_Q = NULL;
My402List *server_Q = NULL;
My402List *completed_Packets_Q = NULL;


/*Function reads a line from input tsfile if -t option is present in th command line input*/
structInput_Values* get_FileLine(FILE *file, int line)
{
	int P, oneBy_LAMBDA, oneBy_MU;
	char buffer1[100];
	if(fgets(buffer1, 100, file) != NULL) {
		char *str_toekn_ptr, *info_ptrToken;
		structInput_Values *info = (structInput_Values *)malloc(sizeof(structInput_Values));
		int i;
		buffer1[strlen(buffer1) - 1] = '\0';
		if(line == 1) {
			for(i = 0; i < strlen(buffer1); i++) {
				if(buffer1[i] == '\t' || buffer1[i] == ' ' || ( buffer1[i] != '1' && buffer1[i] != '2' && buffer1[i] != '3' && buffer1[i] != '4' && buffer1[i] != '5' && buffer1[i] != '6' && buffer1[i] != '7' && buffer1[i] != '8' && buffer1[i] != '9' && buffer1[i] != '0'))
				{
					fprintf(stderr, "ERROR! - [Input file is not in the correct format]\n");
					exit(1);
				}
			}
			n = atoi(buffer1);
			info -> n = n;
			info -> P = 0;
			info -> oneBy_LAMBDA = 0;
			info -> oneBy_MU = 0;
		}
		else{
			info_ptrToken = strtok_r(buffer1, " \t", &str_toekn_ptr);
			if(info_ptrToken != NULL){
				oneBy_LAMBDA = atoi(info_ptrToken);
				info_ptrToken = strtok_r(NULL, " \t", &str_toekn_ptr);
				if(info_ptrToken != NULL){
					P = atoi(info_ptrToken);
					info_ptrToken = strtok_r(NULL, " \t", &str_toekn_ptr);
					if(info_ptrToken != NULL){
						oneBy_MU = atoi(info_ptrToken);
						info_ptrToken = strtok_r(NULL, " \t", &str_toekn_ptr);
						if(info_ptrToken == NULL){
							info -> n = n;
							info -> P = P;
							info -> oneBy_LAMBDA = oneBy_LAMBDA;
							info -> oneBy_MU = oneBy_MU;
						}
						else {//error in file, leading or trailing spaces or tabs
							fprintf(stderr, "ERROR! - [Input file is not in the correct format]\n");
							exit(1);
						}
					}
					else{
						fprintf(stderr, "ERROR! - [Input file is not in the correct format]\n");
						exit(1);
					}
				}
				else{
					fprintf(stderr, "ERROR! - [Input file is not in the correct format]\n");
					exit(1);
				}
			}
			else{

				fprintf(stderr, "ERROR! - [Input file is not in the correct format]\n");
				exit(1);
			}
		}
		return info;
	}
	else{
		//error in file, leading or trailing spaces or tabs
		fprintf(stderr, "ERROR! - [Unable to read file lines]\n");
		exit(1);
	}
}

/*Function converts struct timeval value obtained from gettimeofday to microseconds*/
double getTime_inMicroSeconds(struct timeval temp)
{return ((temp.tv_sec * 1000000) + temp.tv_usec);}
/*Function converts struct timeval value obtained from gettimeofday to milliseconds*/
double converttoMilliSeconds(struct timeval temp)
{return ((temp.tv_sec * 1000000) + temp.tv_usec)/1000;}
/*Function calculates in microseconds, the difference between the current time and the emulation start time*/
double getTime_ToPrint()
{
	struct timeval current_Time;
	if(gettimeofday(&current_Time, 0) == -1){
		//error getting time of the day
		fprintf(stderr, "ERROR! - [Unable to get current system time]\n");
		exit(1);
	}
	return (getTime_inMicroSeconds(current_Time) - getTime_inMicroSeconds(emulation_Start_Time))/1000;
}

/*Function initializes packet with an ID, number of tokens required and the service/transmission time*/
void construct_Packet(struct_PacketData *newPacket, int id, int tokensRequired_Q1toQ2, double transmissionTime)
{
	newPacket -> packet_ID = id;
	newPacket -> tokensRequired_Q1toQ2 = tokensRequired_Q1toQ2;
	newPacket -> transmissionTime = transmissionTime;
}

/*Function adds packet to arrival queue*/
void addTopacket_Arrival_Q(struct_PacketData *newPacket)
{
	if(packet_Arrival_Q != NULL){
		My402ListAppend(packet_Arrival_Q, newPacket);
	}
}

/* Function checks if packet can move from Arrival queue to Server queue by comparing the number of tokens in the bucket and the number required by the packet*/
int checkToken_ToMoveToQ2()
{
	if(packet_Arrival_Q != NULL){
		if(!My402ListEmpty(packet_Arrival_Q)){
			My402ListElem *first = My402ListFirst(packet_Arrival_Q);
			if(((struct_PacketData *)(first -> obj)) -> tokensRequired_Q1toQ2 <= tokenBucket_Counter)
			return 1;
		}
		else
			return 0;
	}
return 0;
}

/*Function transfers packet from Arrival queue to Server queue*/
void transferPacketToQ2()
{
	if(packet_Arrival_Q != NULL && My402ListEmpty(packet_Arrival_Q) != 1){
		My402ListElem *firstPacket = My402ListFirst(packet_Arrival_Q);
		if(server_Q != NULL)
			My402ListAppend(server_Q, firstPacket -> obj);
		My402ListUnlink(packet_Arrival_Q, firstPacket);
	}
}

/*Sets arrival queue departure time and server queue arrival time for packet being transferred*/
void setDepartureArrivalTime(struct timeval temp)
{
	if(packet_Arrival_Q != NULL && My402ListEmpty(packet_Arrival_Q) != 1){
		My402ListElem *firstPacket = My402ListFirst(packet_Arrival_Q);
		((struct_PacketData *)(firstPacket -> obj)) -> departureTimeFromQ1 = temp;
		((struct_PacketData *)(firstPacket -> obj)) -> arrivalTimeToQ2 = temp;
	}
}

/**--------------------------------Thread function for the packet arrival thread-----------------------------------*/
void *processpacket_Arrival_Q(void *whoAmI)
{
	int tokensRequired_Q1toQ2;
	double interarrivalTime, transmissionTime, sleeping_Time = 0, displayTime, processedInterarrival;
	struct timeval current_Time, startOf_PreviousPacket, finishOf_PreviousPacket, justWakeup_Time;
	long double timeInQ1;
	int line_Counter = 0;
	FILE *file = fopen(t, "r");
	structInput_Values *info = NULL;

	pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, 0);  

	if(tFile_check){
		info = get_FileLine(file, 1);
		n = info -> n;
	}
	line_Counter = n;

	while(!signal_break_Flag){
		if(mainPacket_Counter + 1 > n){ // all packets arrived return
			finish_P_ArrivalThread = 1;

			//printf("\n***************************************packet arrivale thread finished @1\n");
			return 0;
		}
		if(tFile_check){
			if(line_Counter > 0){
				info = get_FileLine(file, 2);
				interarrivalTime = info -> oneBy_LAMBDA;
				tokensRequired_Q1toQ2 = info -> P;
				transmissionTime = info -> oneBy_MU;
				line_Counter --;
			}
		}
		else{
			interarrivalTime = 1000.0/(float)lambda;
			tokensRequired_Q1toQ2 = P;
			transmissionTime = 1000.0/(float)mu;
		}
		if(gettimeofday(&finishOf_PreviousPacket, 0) == -1){
			fprintf(stderr, "ERROR! - [Unable to get current system time]\n");
			exit(1);
		}
	

		if(mainPacket_Counter != 0){
			sleeping_Time = interarrivalTime * 1000 - (getTime_inMicroSeconds(finishOf_PreviousPacket) - getTime_inMicroSeconds(startOf_PreviousPacket));
			if(sleeping_Time < 0)
			sleeping_Time = 0;
			//printf("\n***************************************\npacket arrivale thread will sleep\n");
			usleep(sleeping_Time);	
			//printf("\n***************************************\npacket arrivale thread WAKE UP\n");
		}
		if(mainPacket_Counter == 0){
			sleeping_Time = interarrivalTime * 1000;
			//printf("\n***************************************\npacket arrivale thread will sleep\n");
			usleep(sleeping_Time);	
			//printf("\n***************************************\npacket arrivale thread WAKE UP\n");
		}

		if(gettimeofday(&justWakeup_Time, 0) == -1){
			//error getting time of the day
			fprintf(stderr, "ERROR! - [Unable to get current system time]\n");
			exit(1);
		}
		if(mainPacket_Counter == 0){
			processedInterarrival = interarrivalTime;
		}
		else
			processedInterarrival = (getTime_inMicroSeconds(justWakeup_Time) - getTime_inMicroSeconds(startOf_PreviousPacket))/1000;

		totalInterarrival_Time += processedInterarrival;
		struct_PacketData *pack = (struct_PacketData *)malloc(sizeof(struct_PacketData));
		if(pack == NULL){
			fprintf(stderr, "ERROR! - [Unable to allocate memory from the heap]\n");
			exit(1);
		}
		mainPacket_Counter ++;// packet counter increament
		construct_Packet(pack, mainPacket_Counter, tokensRequired_Q1toQ2, transmissionTime);
		if(gettimeofday(&current_Time, 0) == -1){
			//error getting time of the day
			fprintf(stderr, "ERROR! - [Unable to get current system time]\n");
			exit(1);
		}
		if(pack -> tokensRequired_Q1toQ2 > B){ // drop packet as it requires more than given tokens.
			dropPacket_Counter++;//////////////////
			displayTime = getTime_ToPrint();
			flockfile(stdout);
			printf("%012.3fms: p%d arrives, needs %d tokens, inter-arrival time = %.3lfms, dropped\n", displayTime, pack -> packet_ID, pack -> tokensRequired_Q1toQ2, processedInterarrival);
			funlockfile(stdout);
			if(gettimeofday(&startOf_PreviousPacket, 0) == -1){ // only when this tread does not get mutex all packet dropped
				fprintf(stderr, "ERROR! - [Unable to get current system time]\n");
				exit(1);
			}
			continue; // ready to accept next
		}

		if(signal_break_Flag){
			finish_P_ArrivalThread = 1;
			return 0;
		}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		pthread_mutex_lock(&sync_Mutex);
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		pack -> arrivalTimeToQ1 = current_Time;
		displayTime = getTime_ToPrint();
		flockfile(stdout);
		printf("%012.3fms: p%d arrives, needs %d tokens, inter-arrival time = %.3lfms\n", displayTime, pack -> packet_ID, pack -> tokensRequired_Q1toQ2, processedInterarrival);
		funlockfile(stdout);
		if(gettimeofday(&startOf_PreviousPacket, 0) == -1){
			//error getting time of the day
			fprintf(stderr, "ERROR! - [Unable to get current system time]\n");
			exit(1);
		}
		addTopacket_Arrival_Q(pack);//////// my402listappend
		if(gettimeofday(&current_Time, 0) == -1){
			//error getting time of the day
			fprintf(stderr, "ERROR! - [Unable to get current system time]\n");
			exit(1);
		}
		displayTime = getTime_ToPrint();
		flockfile(stdout);
		printf("%012.3fms: p%d enters Q1\n", displayTime, pack -> packet_ID);// packet enters Q1
		funlockfile(stdout);
		if(checkToken_ToMoveToQ2()){
			displayTime = getTime_ToPrint();
			transferPacketToQ2(); // enough token Q1 -> Q2
			//set departure and arrival time for packet being transferred.
			if(gettimeofday(&current_Time, 0) == -1){
				//error getting time of the day
				fprintf(stderr, "ERROR! - [Unable to get current system time]\n");
				exit(1);
			}
			pack -> departureTimeFromQ1 = current_Time;
			pack -> arrivalTimeToQ2 = current_Time;
			timeInQ1 = ((getTime_inMicroSeconds(pack -> departureTimeFromQ1)) - (getTime_inMicroSeconds(pack -> arrivalTimeToQ1)))/1000;
			tokenBucket_Counter -= pack -> tokensRequired_Q1toQ2;
			flockfile(stdout);
			printf("%012.3fms: p%d leaves Q1, time in Q1 = %.3Lfms, token bucket now has %d tokens\n", displayTime, pack -> packet_ID, timeInQ1, tokenBucket_Counter);
			displayTime = getTime_ToPrint();
			printf("%012.3fms: p%d enters Q2\n", displayTime, pack -> packet_ID);
			funlockfile(stdout);
			totalTime_In_Q1 += timeInQ1;
			pthread_cond_signal(&sync_CV_Q2Packet);///// signal to awake thread waiting for CV - sync_CV_Q2Packet = Q2 not empty
		}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		pthread_mutex_unlock(&sync_Mutex);
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	}
finish_P_ArrivalThread = 1;
return 0;
}


/*--------------------------------Thread function for the token depositing thread------------------------------------------*/
void *process_TokenBucket(void *whoAmI)
{
	double sleeping_Time, displayTime;
	long double timeInQ1;
	double tokenArrivalTime = 1000.0/(float)r;
	struct timeval finishOf_PreviousPacket, startOf_PreviousPacket, current_Time;
	pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, 0);
	while(!signal_break_Flag){
		if(finish_P_ArrivalThread == 1  && My402ListEmpty(packet_Arrival_Q)){
				finish_TokenThread = 1;
				//printf("***************************************Token bucket thread finished @1\n");
				return 0;
		}

		if(gettimeofday(&finishOf_PreviousPacket, 0) == -1){
			fprintf(stderr, "ERROR! - [Unable to get current system time]\n");
			exit(1);
		}
		if(everyToken_Counter != 0){ //if there are tokens ...sleep for time to generae next token
			sleeping_Time = tokenArrivalTime * 1000 - (getTime_inMicroSeconds(finishOf_PreviousPacket) - getTime_inMicroSeconds(startOf_PreviousPacket));
			if(sleeping_Time < 0)
				sleeping_Time = 0;
			usleep(sleeping_Time);
		}
		if(everyToken_Counter == 0){ //no token then sleep i.e. first time for tokenArrivalTime
			sleeping_Time = tokenArrivalTime * 1000;
			usleep(sleeping_Time);
		}

		if(finish_P_ArrivalThread == 1 && My402ListEmpty(packet_Arrival_Q)){
				finish_TokenThread = 1;
				//if(!My402ListEmpty(server_Q))
					pthread_cond_broadcast(&sync_CV_Q2Packet);
				//printf("***************************************Token bucket thread finished @2\n");
				return 0;
		}

		if(signal_break_Flag){
			continue;
		}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		pthread_mutex_lock(&sync_Mutex);
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		if(gettimeofday(&startOf_PreviousPacket, 0) == -1){
			//error getting time of the day
			fprintf(stderr, "ERROR! - [Unable to get current system time]\n");
			exit(1);
		}
		everyToken_Counter ++;
		displayTime = getTime_ToPrint();
		if(tokenBucket_Counter + 1 <= B) {
			tokenBucket_Counter ++;
			flockfile(stdout);
			printf("%012.3fms: token t%d arrives, token bucket now has %d tokens\n", displayTime, everyToken_Counter, tokenBucket_Counter);
			funlockfile(stdout);
		}
		else {// exceeded the max count ... token dropped
			flockfile(stdout);
			printf("%012.3fms: token t%d arrives, dropped\n", displayTime, everyToken_Counter);
			funlockfile(stdout);
			droppedToken_Counter ++;
		}
		if(checkToken_ToMoveToQ2()){
			displayTime = getTime_ToPrint();
			if(gettimeofday(&current_Time, 0) == -1){
				fprintf(stderr, "ERROR! - [Unable to get current system time]\n");
				exit(1);
			}
			My402ListElem *firstPacket = My402ListFirst(packet_Arrival_Q);
			timeInQ1 = ((getTime_inMicroSeconds(current_Time)) - (getTime_inMicroSeconds(((struct_PacketData *)(firstPacket -> obj)) -> arrivalTimeToQ1)))/1000;
			totalTime_In_Q1 += timeInQ1;
			tokenBucket_Counter -= ((struct_PacketData *)(firstPacket -> obj)) -> tokensRequired_Q1toQ2;
			flockfile(stdout);
			printf("%012.3fms: p%d leaves Q1, time in Q1 = %.3Lfms, token bucket now has %d tokens\n",displayTime, ((struct_PacketData *)(firstPacket ->obj))-> packet_ID, timeInQ1, tokenBucket_Counter);
			displayTime = getTime_ToPrint();
			printf("%012.3fms: p%d enters Q2\n", displayTime, ((struct_PacketData *)(firstPacket -> obj)) -> packet_ID);
			funlockfile(stdout);
			setDepartureArrivalTime(current_Time);
			transferPacketToQ2(); //<-------------------------------------- transfer packet from Q1 to Q2

			pthread_cond_signal(&sync_CV_Q2Packet);// signal to awake thread waiting for CV - sync_CV_Q2Packet = Q2 not empty
		}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		pthread_mutex_unlock(&sync_Mutex);
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	}
finish_TokenThread = 1;
//printf("***************************************Token bucket thread finished @3\n");
return 0;
}

/*Function to unlink packet being serviced from Q2*/
void transferPacketToServer()
{
	if(server_Q != NULL && My402ListEmpty(server_Q) != 1){ // server_Q exists and is not empty?
		My402ListElem *firstPacket = My402ListFirst(server_Q) -> obj;
		My402ListUnlink(server_Q, firstPacket);
	}
}

/*-------------------------------------Thread function for the server processing--------------------------------------------*/
void *processserver_Q(void *whoAmI)
{
double sleeping_Time = 0, timeInQ2, timeInSystem, displayTime, serviceTime;
struct timeval current_Time;
char *who = whoAmI;
while(1)
{
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	pthread_mutex_lock(&sync_Mutex);
	//printf("\nServer%s have mutex\n\n",who);
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	while(My402ListEmpty(server_Q) && (mainPacket_Counter + 1) < n && !signal_break_Flag){ // server_Q empty?
		if ((mainPacket_Counter + 1) < n)
		//printf("\nServer%s waiting for CV - sync_CV_Q2Packet\n",who);
		pthread_cond_wait(&sync_CV_Q2Packet, &sync_Mutex);// getting blocked?    //wait for signal
		//printf("\nServer%s out of wait CV - sync_CV_Q2Packet\n",who);
	}
	if(My402ListEmpty(server_Q) && finish_TokenThread && !signal_break_Flag){
			//pthread_cond_broadcast(&sync_CV_Q2Packet);
			pthread_cond_broadcast(&sync_CV_Q2Packet);
			pthread_mutex_unlock(&sync_Mutex);
			printf("***************************************Server%s thread finished @1\n",who);
			return 0;
	}
	if(My402ListEmpty(server_Q) && finish_TokenThread && signal_break_Flag){
			//pthread_cond_broadcast(&sync_CV_Q2Packet);
			pthread_mutex_unlock(&sync_Mutex);
			pthread_cond_signal(&sync_CV_Q2Packet);
			printf("***************************************Server%s thread finished @new break\n",who);
			return 0;
	}
//-------------------------------------------------------------------------------------------------------
	if(!My402ListEmpty(server_Q) && (!signal_break_Flag)){
		struct_PacketData *serverQ_Front = (struct_PacketData *)(My402ListFirst(server_Q) -> obj);
		sleeping_Time = serverQ_Front -> transmissionTime;
		if(gettimeofday(&current_Time, 0) == -1){
			//error getting time of the day
			fprintf(stderr, "ERROR! - [Unable to get current system time]\n");
			exit(1);
		}
		serverQ_Front -> departureTimeFromQ2 = current_Time;
		displayTime = getTime_ToPrint();
		timeInQ2 = (getTime_inMicroSeconds(serverQ_Front -> departureTimeFromQ2) - getTime_inMicroSeconds(serverQ_Front -> arrivalTimeToQ2))/1000;
		totalTime_In_Q2 += timeInQ2;

		My402ListUnlink(server_Q, My402ListFirst(server_Q));

		flockfile(stdout);
		printf("%012.3fms: p%d leaves Q2, time in Q2 = %.3lfms\n", displayTime, serverQ_Front -> packet_ID, timeInQ2);
		printf("%012.3fms: p%d begins service at S%s, requesting %.3lfms of service\n", displayTime, serverQ_Front -> packet_ID, who, sleeping_Time);
		funlockfile(stdout);


		if(*who=='1'){
			number_S1_delivered++;//flockfile(stdout);printf("\n server1 count : %d\n", number_S1_delivered);funlockfile(stdout);
		}
		if(*who=='2'){
			number_S2_delivered++;//flockfile(stdout);printf("\n server2 count : %d\n", number_S2_delivered);funlockfile(stdout);
		}
		if(*who=='3'){
			number_S3_delivered++;//flockfile(stdout);printf("\n server1 count : %d\n", number_S1_delivered);funlockfile(stdout);
		}
		if(*who=='4'){
			number_S4_delivered++;//flockfile(stdout);printf("\n server1 count : %d\n", number_S1_delivered);funlockfile(stdout);
		}
		if(*who=='5'){
			number_S5_delivered++;//flockfile(stdout);printf("\n server1 count : %d\n", number_S1_delivered);funlockfile(stdout);
		}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		//printf("\nServer%s have UNLOCK mutex\n\n",who);	
		pthread_mutex_unlock(&sync_Mutex);
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		usleep((sleeping_Time) * 1000);
		if(gettimeofday(&current_Time, 0) == -1){
			fprintf(stderr, "ERROR! - [Unable to get current system time]\n");
			exit(1);
		}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		pthread_mutex_lock(&sync_Mutex);
		//printf("\nServer%s have mutex\n\n",who);
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		serviceTime = (getTime_inMicroSeconds(current_Time) - getTime_inMicroSeconds(serverQ_Front -> departureTimeFromQ2))/1000;
		if(*who=='1'){
			totalService_Time_S1 += serviceTime;
		}
		if(*who=='2'){
			totalService_Time_S2 += serviceTime;
		}
		if(*who=='3'){
			totalService_Time_S3 += serviceTime;
		}
		if(*who=='4'){
			totalService_Time_S4 += serviceTime;
		}
		if(*who=='5'){
			totalService_Time_S5 += serviceTime;
		}

		timeInSystem = (getTime_inMicroSeconds(current_Time) - getTime_inMicroSeconds(serverQ_Front -> arrivalTimeToQ1))/1000;
		displayTime = getTime_ToPrint();
		flockfile(stdout);
		printf("%012.3fms: p%d departs from S%s, service time = %.3lfms, time in system = %.3lfms\n", displayTime, serverQ_Front -> packet_ID,who, serviceTime, timeInSystem);
		funlockfile(stdout);
		totalTime_In_System += timeInSystem;
		totalTime_In_System_Sqrd += timeInSystem * timeInSystem;
	}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	//printf("\nServer%s have UNLOCK mutex\n\n",who);
	pthread_mutex_unlock(&sync_Mutex);
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	if(finish_TokenThread && signal_break_Flag ){//&& My402ListEmpty(server_Q))
			printf("***************************************Server%s thread finished @2\n",who);
		return 0;
	}
}
			printf("***************************************Server%s thread finished @end\n",who);
return 0;
}

/*Function to calculate required statistics as per the spec*/
void removePacketFrom_Server(){
	if(completed_Packets_Q != NULL && My402ListEmpty(completed_Packets_Q) != 1){
		My402ListElem *firstPacket = My402ListFirst(completed_Packets_Q) -> obj;
		My402ListUnlink(server_Q, firstPacket);
	}
}
/**------------------------------------Function to calculate required statistics as per the spec-------------------------------------*/
void getAll_Statists() {
	double token_Drop_Probability, packet_Drop_Probability, avgTimeSpent_InSystem, avgPacketService_Time, avgPacketInterArri_Time;
	double avgPackets_inQ1, avgPackets_inQ2, avgPackets_atS1,avgPackets_atS2, avgPackets_atS3,avgPackets_atS4,avgPackets_atS5, SD_Time_onSystem;
	double temp1, temp2, temp3, temp4;
	totalService_Time = totalService_Time_S1 + totalService_Time_S2 + totalService_Time_S3 +totalService_Time_S4 + totalService_Time_S5;

	token_Drop_Probability = (double)droppedToken_Counter / (double)everyToken_Counter;
	packet_Drop_Probability = (double)dropPacket_Counter / (double)mainPacket_Counter;

	avgTimeSpent_InSystem = (totalTime_In_System / 1000) / (mainPacket_Counter - dropPacket_Counter - removedPacket_Counter);

	avgPacketService_Time = (totalService_Time / 1000) / (mainPacket_Counter - dropPacket_Counter - removedPacket_Counter);

	avgPacketInterArri_Time = (totalInterarrival_Time / 1000) / mainPacket_Counter;

	avgPackets_inQ1 = (totalTime_In_Q1) / totalEmulation_RunTime;
	avgPackets_inQ2 = (totalTime_In_Q2) / totalEmulation_RunTime;

	avgPackets_atS1 = (totalService_Time_S1) / totalEmulation_RunTime;
	avgPackets_atS2 = (totalService_Time_S2) / totalEmulation_RunTime;
	avgPackets_atS3 = (totalService_Time_S3) / totalEmulation_RunTime;
	avgPackets_atS4 = (totalService_Time_S4) / totalEmulation_RunTime;
	avgPackets_atS5 = (totalService_Time_S5) / totalEmulation_RunTime;

	temp1 = totalTime_In_System_Sqrd/(1000 * 1000);
	temp2 = mainPacket_Counter - dropPacket_Counter - removedPacket_Counter;
	temp3 = temp1 / temp2;

	temp4 = avgTimeSpent_InSystem * avgTimeSpent_InSystem;

	SD_Time_onSystem = sqrt(temp3 - temp4);
//---------------------------------------------------------------------------------------------------------------
	printf("\nStatistics:\n\n");
	if(mainPacket_Counter == 0)
		printf("\taverage packet inter-arrival time = N/A [No packet arrived in the system]\n");
	else
		printf("\taverage packet inter-arrival time = %.6g\n", avgPacketInterArri_Time);
//---------------------------------------------------------------------------------------------------------------
	if((mainPacket_Counter - dropPacket_Counter - removedPacket_Counter) == 0){
		if(mainPacket_Counter == 0)//packet didnt reach in system 
			printf("\taverage packet service time = N/A [No packet arrived in the system]\n");
		else//packet didnt reach to server
			printf("\taverage packet service time = N/A [No packet reached till server]\n");
	}
	else // showing packet service time
		printf("\taverage packet service time = %.6g\n", avgPacketService_Time);
//---------------------------------------------------------------------------------------------------------------
	if(totalEmulation_RunTime == 0){
		printf("\n\taverage number of packets in Q1 = N/A [Emulation time is zero}\n");
		printf("\taverage number of packets in Q2 = N/A [Emulation time is zero]\n");	
		printf("\taverage number of packets in S1 = N/A [Emulation time is zero]\n");
		printf("\taverage number of packets in S2 = N/A [Emulation time is zero]\n");
		printf("\taverage number of packets in S3 = N/A [Emulation time is zero]\n");
		printf("\taverage number of packets in S4 = N/A [Emulation time is zero]\n");
		printf("\taverage number of packets in S5 = N/A [Emulation time is zero]\n");

	}
	else{
		printf("\n\taverage number of packets in Q1 = %.6g\n", avgPackets_inQ1);
		printf("\taverage number of packets in Q2 = %.6g\n", avgPackets_inQ2);
		printf("\taverage number of packets in S1 = %.6g\n", avgPackets_atS1);
		printf("\taverage number of packets in S2 = %.6g\n", avgPackets_atS2);
		printf("\taverage number of packets in S3 = %.6g\n", avgPackets_atS3);
		printf("\taverage number of packets in S4 = %.6g\n", avgPackets_atS4);
		printf("\taverage number of packets in S5 = %.6g\n", avgPackets_atS5);

	}
//---------------------------------------------------------------------------------------------------------------
	if((mainPacket_Counter - dropPacket_Counter - removedPacket_Counter) == 0){
		if(mainPacket_Counter == 0)
			printf("\n\taverage time a packet spent in system = N/A [No packet arrived in the system]\n");
		else
			printf("\n\taverage time a packet spent in system = N/A [No packet reached till server]\n");
	}
	else
		printf("\n\taverage time a packet spent in system = %.6g\n", avgTimeSpent_InSystem);
//---------------------------------------------------------------------------------------------------------------
	if((mainPacket_Counter - dropPacket_Counter - removedPacket_Counter) == 0){
		if(mainPacket_Counter == 0)
			printf("\tstandard deviation for time spent in system = N/A [No packet arrived in the system]\n");
		else
			printf("\tstandard deviation for time spent in system = N/A [No packet reached till server]\n");
	}
	else
		printf("\tstandard deviation for time spent in system = %.6g\n", SD_Time_onSystem);
//---------------------------------------------------------------------------------------------------------------
	if(everyToken_Counter == 0)
		printf("\n\ttoken drop probability = N/A [No token arrived in the system]\n");
	else	
		printf("\n\ttoken drop probability = %.6g\n", token_Drop_Probability);
	if(mainPacket_Counter == 0)
		printf("\tpacket drop probability = N/A [No packet arrived in the system]\n");
	else
		printf("\tpacket drop probability = %.6g\n\n", packet_Drop_Probability);
//---------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------
}





/**------------------------------------------Handling the SIGINT interrupt-----------------------------------------*/
void *signalMonitor_Interpt(void *whoAmI) {
	int returnedSigNumber;
	sigwait(&intrpt_Signal, &returnedSigNumber); //wait for 1of signalset '&intrpt_Signal' become pending. removes it from the pending list of signals & will returns the signal number in returnedSigNumber

	My402ListElem *removing = NULL;
	double displayTime;
	int remaining;

	signal_break_Flag = 1;
	finish_P_ArrivalThread = 1;
	finish_TokenThread = 1;

	pthread_cancel(packetMonitorThread); 	//printf("\n1] Canceling packet monitor thread");
	pthread_cancel(tokenMonitorThread);	//printf("\n2] Canceling Token monitor thread");

	flockfile(stdout); // lock stdout for printing
	printf("\n");
	funlockfile(stdout);

//-------------------- mutex lock ----------------------------
	pthread_mutex_lock(&sync_Mutex);

	if(My402ListLength(server_Q) == 0)
		pthread_cond_broadcast(&sync_CV_Q2Packet);

	removedPacket_Counter = My402ListLength(packet_Arrival_Q);
	remaining = My402ListLength(packet_Arrival_Q);
	removing = My402ListFirst(packet_Arrival_Q);
	while(remaining != 0){
		displayTime = getTime_ToPrint();
		flockfile(stdout);
		printf("%012.3fms: p%d removed from Q1\n", displayTime, ((struct_PacketData *)(removing -> obj)) -> packet_ID);
		funlockfile(stdout);
		removing = My402ListNext(packet_Arrival_Q, removing);
		remaining--;
	}

	pthread_mutex_unlock(&sync_Mutex);
//-------------------- mutex unlock --------------------------
return 0;
}

/**----------------------------------------------Function for creation of threads----------------------------------*/
void getAll_Threads() {
	int errorNumber, remaining;
	double displayTime;
	My402ListElem *removing = NULL;
	if(gettimeofday(&emulation_Start_Time, 0) == -1){
		fprintf(stderr, "ERROR! - [Unable to get current system time]\n");
		exit(1);
	}
	//emulation begin and SIGINT and masking sig
	printf("%012.3fms: emulation begins\n", 0.0f);

	sigemptyset(&intrpt_Signal); //if error print else continue ***** init & empty signla set : error returns "-1" in errno
	sigaddset(&intrpt_Signal, SIGINT); // intrpt_Signal - signal set, SIGINT - signal number
	pthread_sigmask(SIG_BLOCK, &intrpt_Signal, 0);  // SIG_BLOCK - way in which set is changed, &intrpt_Signal - 

	// signal handler
	errorNumber = pthread_create(&signalMonitorThread, NULL, signalMonitor_Interpt, NULL);
	if(errorNumber != 0){
		fprintf(stderr, "ERROR! - [Unable to create thread]\n");
		exit(1);
	}
	// processing arrival Q
	errorNumber = pthread_create(&packetMonitorThread, NULL, processpacket_Arrival_Q, NULL);
	if(errorNumber != 0){
		fprintf(stderr, "ERROR! - [Unable to create thread]\n");
		exit(1);
	}
	// process token bucket Q
	errorNumber = pthread_create(&tokenMonitorThread, NULL, process_TokenBucket, NULL);
	if(errorNumber != 0){
		fprintf(stderr, "ERROR! - [Unable to create thread]\n");
		exit(1);
	}




	// process server1 Q
	errorNumber = pthread_create(&serverMonitorThread1, NULL, processserver_Q, "1");
	if(errorNumber != 0){
		fprintf(stderr, "ERROR! - [Unable to create 1st thread]\n");
		exit(1);
	}
	// process server2 Q
	errorNumber = pthread_create(&serverMonitorThread2, NULL, processserver_Q, "2");
	if(errorNumber != 0){
		fprintf(stderr, "ERROR! - [Unable to create 2nd thread]\n");
		exit(1);
	}

	// process server3 Q
	errorNumber = pthread_create(&serverMonitorThread3, NULL, processserver_Q, "3");
	if(errorNumber != 0){
		fprintf(stderr, "ERROR! - [Unable to create 3rd thread]\n");
		exit(1);
	}

	// process server4 Q
	errorNumber = pthread_create(&serverMonitorThread4, NULL, processserver_Q, "4");
	if(errorNumber != 0){
		fprintf(stderr, "ERROR! - [Unable to create 3rd thread]\n");
		exit(1);
	}
	// process server5 Q
	errorNumber = pthread_create(&serverMonitorThread5, NULL, processserver_Q, "5");
	if(errorNumber != 0){
		fprintf(stderr, "ERROR! - [Unable to create 3rd thread]\n");
		exit(1);
	}

	// joinig all threads
	pthread_join(packetMonitorThread, 0);
	pthread_join(tokenMonitorThread, 0);
	pthread_join(serverMonitorThread1, 0);
	pthread_join(serverMonitorThread2, 0);
	pthread_join(serverMonitorThread3, 0);
	pthread_join(serverMonitorThread4, 0);
	pthread_join(serverMonitorThread5, 0);

	remaining = My402ListLength(server_Q); // a is lenght of server Q
	removing = My402ListFirst(server_Q); // i is my402list element

	//serve the packets i.e. removing from server queue "Q2"
	while(remaining != 0) {
		displayTime = getTime_ToPrint();
		printf("%012.3fms: p%d removed from Q2\n", displayTime, ((struct_PacketData *)(removing -> obj)) -> packet_ID);
		removing = My402ListNext(server_Q, removing);
		remaining--;
	}

	displayTime = getTime_ToPrint();
	if(gettimeofday(&emulation_End_Time, 0) == -1){
		fprintf(stderr, "ERROR! - [Unable to get current system time]\n");
		exit(1);
	}
	printf("%012.3fms: emulation ends\n", displayTime);
	totalEmulation_RunTime = displayTime;
}

//==================================================================================================================================================
                       /////////////////////////////////////////// START PROGRAM ///////////////////////////////////////////////////
//==================================================================================================================================================

int main(int argc, char *argv[]){
	int i,z=1;
	//char buffer1[100];

	for(i = 0; i < argc-1; i++){  // check for '0' value of inputs
		if(strcmp(argv[i], "-lambda") == 0){
			if(strcmp(argv[i+1],"-lambda") * strcmp(argv[i+1],"-mu") * strcmp(argv[i+1],"-t") * strcmp(argv[i+1],"-P") * strcmp(argv[i+1],"-B") * strcmp(argv[i+1],"-r") * strcmp(argv[i+1],"-n") ==0)
			{
				fprintf (stderr,"ERROR! : [Missing parameter value for 'lambda']\n"); //errno ==2
				exit(1);						
			}
			lambda = atof(argv[i+1]);
			if(lambda < 0)
			{
				fprintf(stderr, "ERROR! : ['lambda' must be a positive real number]\n");
				exit(1);
			}
			// as per specification
			if((1/lambda) > 10)
				lambda = 0.1;
		}
		if(strcmp(argv[i], "-mu") == 0)//------------------------------------------------------------------------------------------
		{

if(strcmp(argv[i+1],"-lambda") * strcmp(argv[i+1],"-mu") * strcmp(argv[i+1],"-t") * strcmp(argv[i+1],"-P") * strcmp(argv[i+1],"-B") * strcmp(argv[i+1],"-r") * strcmp(argv[i+1],"-n") ==0)
			{
				fprintf (stderr,"ERROR! : [Missing parameter value for 'mu']\n"); //errno ==2
				//fclose(fpcheck);
				exit(1);						
			}
			mu = atof(argv[i+1]);
			if(mu < 0)
			{
				fprintf(stderr, "ERROR! : ['mu' must be a positive real number]\n");
				exit(1);
			}
			// as per specification
			if((1/mu) > 10)
			mu = 0.1;
		}
		if(strcmp(argv[i], "-B") == 0)//------------------------------------------------------------------------------------------
		{
if(strcmp(argv[i+1],"-lambda") * strcmp(argv[i+1],"-mu") * strcmp(argv[i+1],"-t") * strcmp(argv[i+1],"-P") * strcmp(argv[i+1],"-B") * strcmp(argv[i+1],"-r") * strcmp(argv[i+1],"-n") ==0)
			{
				fprintf (stderr,"ERROR! : [Missing parameter value for 'B']\n"); //errno ==2
				//fclose(fpcheck);
				exit(1);						
			}
			B = atoi(argv[i+1]);
		}

		if(B < 0 || B > MAX_VAL)
		{
			fprintf(stderr, "ERROR! : ['B' must be a positive integer & smaller than 2147483647]\n");
			exit(1);
		}
			// as per specification
		if(strcmp(argv[i], "-P") == 0)//------------------------------------------------------------------------------------------
		{
if(strcmp(argv[i+1],"-lambda") * strcmp(argv[i+1],"-mu") * strcmp(argv[i+1],"-t") * strcmp(argv[i+1],"-P") * strcmp(argv[i+1],"-B") * strcmp(argv[i+1],"-r") * strcmp(argv[i+1],"-n") ==0)
			{
				fprintf (stderr,"ERROR! : [Missing parameter value for 'P']\n"); //errno ==2
				//fclose(fpcheck);
				exit(1);						
			}
			P = atoi(argv[i+1]);
		}
		if(P < 0 || P > MAX_VAL)
		{
			fprintf(stderr, "ERROR! : ['P' must be a positive integer & smaller than 2147483647]\n");
			exit(1);
		}
			// as per specification
		if(strcmp(argv[i], "-r") == 0)//------------------------------------------------------------------------------------------
		{
if(strcmp(argv[i+1],"-lambda") * strcmp(argv[i+1],"-mu") * strcmp(argv[i+1],"-t") * strcmp(argv[i+1],"-P") * strcmp(argv[i+1],"-B") * strcmp(argv[i+1],"-r") * strcmp(argv[i+1],"-n") ==0)
			{
				fprintf (stderr,"ERROR! : [Missing parameter value for 'r']\n"); //errno ==2
				//fclose(fpcheck);
				exit(1);						
			}
			r = atof(argv[i+1]);
			if(r < 0)
			{
				fprintf(stderr, "ERROR! : ['r' must be a positive real number]\n");
				exit(1);
			}
			// as per specification
			if((1/r) > 10) /*If 1/r is greater than 10 seconds, please use an inter-token-arrival time of 10 seconds */
				r = 0.1;
		}
		if(strcmp(argv[i], "-n") == 0)//------------------------------------------------------------------------------------------
		{
if(strcmp(argv[i+1],"-lambda") * strcmp(argv[i+1],"-mu") * strcmp(argv[i+1],"-t") * strcmp(argv[i+1],"-P") * strcmp(argv[i+1],"-B") * strcmp(argv[i+1],"-r") * strcmp(argv[i+1],"-n") ==0)
			{
				fprintf (stderr,"ERROR! : [Missing parameter value for 'n']\n"); //errno ==2
				//fclose(fpcheck);
				exit(1);						
			}
			n = atoi(argv[i+1]);
		}
		if(n < 0 || n > MAX_VAL)
		{
			fprintf(stderr, "ERROR! : ['n' must be a positive integer and smaller than 2147483647]\n");
			exit(1);
		}
			// as per specification

		if ((argc == 2) && strcmp(argv[i+1],"-t")==0)
		{
			fprintf(stderr, "ERROR! : [malformed command]\n"); // ODD/EXTRA NUMBER OF ARGUMENTS -only 2 argc with -t
			exit(1);
		}
		if(strcmp(argv[i], "-t") == 0)//------------------------------------------------------------------------------------------
		{
			tFile_check = 1;
			if(argv[i+1] == NULL)
			{
				fprintf(stderr, "ERROR!%d : [File name is not provided]\n",errno);
				exit(1);
			}
			else
			{
				FILE *fpcheck;
				fpcheck = fopen(argv[i+1], "r");
				if (errno ==EACCES)
				{
					fprintf (stderr,"Error : [Input file '%s' cannot be opened - access denies]\n",argv[i+1]); //errno ==2
					//fclose(fpcheck);
					exit(1);
				}

				if (fpcheck == NULL) 
				{
					if(strcmp(argv[i+1],"-lambda") * strcmp(argv[i+1],"-mu") * strcmp(argv[i+1],"-t") * strcmp(argv[i+1],"-P") * strcmp(argv[i+1],"-B") * strcmp(argv[i+1],"-r") * strcmp(argv[i+1],"-n") ==0)
					{
					fprintf (stderr,"ERROR! : [File name is not provided]\n"); //errno ==2
					//fclose(fpcheck);
					exit(1);						
					}

					fprintf (stderr,"Error : [Input file '%s' does not exist]\n",argv[i+1]); //errno ==2
					//fclose(fpcheck);
					exit(1);
				}

				if(chdir(argv[i+1])==0) 
				{
					fprintf (stderr,"Error%d : [ '%s' is directory]\n",errno,argv[2]); // errno == 0
					fclose(fpcheck);
					exit(1);
				}
				t = strdup(argv[i+1]);
				//fgets(buffer1, 100, fpcheck);
				/*if (errno == 20){
					fprintf (stderr,"Error : Input file is not in the right format\n"); //errno ==2
					fclose(fpcheck);exit(1);}*/
			}
			// as per specification
		}
	}

//----------------------------------------------------------------------------------------------------------------------------
	if ((argc % 2 == 0 || argc > 15) && strcmp(argv[1],"-t")!=0){
		fprintf(stderr, "ERROR! : [malformed command]\n"); // ODD/EXTRA NUMBER OF ARGUMENTS - outside of all check
		exit(1);
	}
	z=1;
	while(z < argc && argc > 1) {     /////// check for '0' value of inputs----------------------------------------------
		if(strcmp(argv[z],"-lambda") * strcmp(argv[z],"-mu") * strcmp(argv[z],"-t") * strcmp(argv[z],"-P") * strcmp(argv[z],"-B") * strcmp(argv[z],"-r") * strcmp(argv[z],"-n") ==0)
		{}
		else{
			fprintf(stderr, "ERROR! : [malformed command]\n"); // ATTRIBUTE NAME MISSMATECH
			exit(1);
		}
	z=z+2;
	}
//----------------------------------------------------------------------------------------------------------------------------
	FILE *file1 = fopen(t, "r");
	structInput_Values *info = NULL; // 1/lambda, 1/mu, P, n structure
	if(tFile_check){
		info = get_FileLine(file1, 1);
		n = info->n;
		fclose(file1);
	}
	printf("\nEmulation Parameters:");
	printf("\n\tnumber to arrive = %d", n);
	if(!tFile_check)
		printf("\n\tlambda = %.3g", lambda);
	if(!tFile_check)
		printf("\n\tmu = %.3g", mu);
	printf("\n\tr = %.3g", r);
	printf("\n\tB = %d", B);
	if(!tFile_check)
		printf("\n\tP = %d", P);
	if(tFile_check)
		printf("\n\ttsfile = %s", t);
	printf("\n\n");

	packet_Arrival_Q = (My402List *)malloc(sizeof(My402List));
	if(packet_Arrival_Q == NULL){
		fprintf(stderr, "ERROR! : [Memory allocation failuar for packet arrival Queue]\n");
		exit(1);
	}
	else
	My402ListInit(packet_Arrival_Q);
	server_Q = (My402List *)malloc(sizeof(My402List));
	if(server_Q == NULL){
		fprintf(stderr, "ERROR! : [Memory allocation failuar for packet server Queue\n");
		exit(1);
	}
	else
		My402ListInit(server_Q);

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	getAll_Threads();
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	getAll_Statists();
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

return 0;
}
