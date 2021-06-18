/*
 * context switching.c
 *
 *  Created on: 18 Jun 2021
 *      Author: Grant
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include <time.h>
#include <sys/time.h>
#include <sys/types.h>
#include <dirent.h>

struct pcb
{
    int p_id;
    double insNo;
    char p_name[50];
    double arrival_time;
    double cpu_burst_time;
    int priority;
};

int noOfFiles=0;
int time_slice=2;

struct pcb p_queue[10];

void pcbinitialise()
{
	srand(time(0));
	time_t t=time(0);

	struct dirent *dp;
	char pname[10][100];

	DIR *fd; // pointer to directry
	fd=opendir("processes"); // open processes directry

	while((dp = readdir(fd)) != NULL)
	{

		// skip self and parent
		if (!strcmp(dp->d_name, ".") || !strcmp(dp->d_name, ".."))
		{
			continue;
		}

		// copy the relative file name in pname[i]
		strcpy(pname[noOfFiles],"processes/");
		strcat(pname[noOfFiles],dp->d_name);

		printf("%s\n", pname[noOfFiles]);

		// increment the index for "pname" array
		noOfFiles++;
	}
	closedir(fd);

	FILE *f1, *fplog;
	double count=0;
	char c;
	int i;

	fplog=fopen("logs_rr_with_thread","w");
	fprintf(fplog, "<--- START of Round Robin Log file (with Threads) --->\n\n");
	fprintf(fplog, "Time slice = %d instructions\n\nReady Queue:\n\nFront -> ", time_slice);

	for(i=0;i<noOfFiles;i++)
	{
		count=0;

		if(i==noOfFiles-1)
		{
		    fprintf(fplog, "%s\n\n----------", pname[i]);
		}
		else
		{
		    fprintf(fplog,"%s | ",pname[i]);
		}

		// store values in pcb parameters
		p_queue[i].p_id=(i+1);
		p_queue[i].insNo=0;
		strcpy(p_queue[i].p_name,pname[i]);
		p_queue[i].arrival_time=difftime(time(0),t);
		p_queue[i].priority=rand()%10;

		// calculate cpu burst time
		f1=fopen(pname[i],"r");
		for (c = getc(f1); c != EOF; c = getc(f1))
		{
		    if (c == '\n')
		    {
		    	count = count + 1;
		    }
		}
		p_queue[i].cpu_burst_time=count;

		fclose(f1);
	}
	fprintf(fplog, "\n");
	fclose(fplog);
}

void decodeExec(char instruction[], FILE *fplog)
{
	fprintf(fplog, "Instruction: %s", instruction);

	char *ch;

	// maximum no. of operatin name is 10 and each operation name length is BUFSIZ
	char insParams[10][BUFSIZ];

	// opcode op1 op2 - 3 tokens in instruction - store in array
	int i=0,j=0,k=0,l=0;

	// split 'instruction' into series of tokens with delimiter ' ' - store all tokens in "insParams" array
	ch=strtok(instruction," ");
	while (ch != NULL)
	{
		strcpy(insParams[i],ch); // store the token in 'insParams' array
		ch=strtok(NULL," "); // fetch the next token
		i++;
	}

	// tempT contains insParams[3] without '\n' character at end
	char tempT[BUFSIZ];
	for(l=0;insParams[3][l]!='\n';l++)
	{
		tempT[l]=insParams[3][l];
	}
	tempT[l] = '\0';

	// At this point, insParams contains the operands in form of tokens
	char operation[4];

	// copy instruction code to char array 'operation'
	strcpy(operation,insParams[0]);

	// open opcodes.txt since we need to find opcode of the instruction
	FILE *fin;
	fin=fopen("variables.txt", "r");

	// store 1 line data from 'variables.txt' file in 'line' variable
	char line[BUFSIZ];

	char var1[BUFSIZ], var2[BUFSIZ];
	int val1,val2;
	int var1Found=0, var2Found=0;

	// fetch 1 line from "variables.txt" file
	while(fgets(line,sizeof(line),fin) && (!(var1Found && var2Found)))
	{
		// get the first token
	   	ch = strtok(line, " ");
	   	if(strcmp(ch,insParams[1])==0)
	   	{
	   		var1Found=1;

	   		//get the next token
	   		ch = strtok(NULL, " ");
	   		strcpy(var1, ch);
	   	}

	   	if(strcmp(ch,insParams[2])==0)
	   	{
	   		var2Found = 1;

	   		//get the next token
	   		ch = strtok(NULL, " ");
	   		strcpy(var2, ch);
	   	}
	}

	// convert values of val1 and val2 to int as they are of type 'string'
	// if var1 was not found meaning there was an integer passed in place of var1
	if(!var1Found)
	{
	   	val1=atoi(insParams[1]);
	}
	else
	{
	   	val1=atoi(var1);
	}

	// if var2 was not found meaning there was an integer passed in place of var2
	if(!var2Found)
	{
	   	val2=atoi(insParams[2]);
	}
	else
	{
	   	val2=atoi(var2);
	}
	fclose(fin); // close "variables.txt" file

	int result;

	// --- operations are also given a code - to fetch opcodes --- //
	// open opcodes.txt since we need to find opcode of the instruction
	fin=fopen("opcode.txt", "r");

	// array to store all opcodes; max. no. of opcodes taken as 10; pointer because opcode
	char opNames[10][4];
	int opCodes[10];

	i=0;

	// store all opcodes in "opcodes[]"
	while(fgets(line,sizeof(line),fin))
	{
		// get the first token
	   	ch = strtok(line, " ");
	   	strcpy(opNames[i], ch);

	   	//get the next token
	   	ch = strtok(NULL, " ");

	   	opCodes[i]=atoi(ch);

	   	i++;
	}

	int finalOpCode;
	for(j=0;j<5;j++)
	{
		if(strcmp(opNames[j], operation)==0)
		{
			finalOpCode=opCodes[j];
		}
	}
	fclose(fin);

	fprintf(fplog, "Decoded instruction: %d %d %d %s\n", finalOpCode, val1, val2, tempT);
}

// define max. no. of processes
#define MAX 5

// for storing the number of instructions left in particular file
double noOfTimes[MAX];

// for switching threads
int turn = 0;

// file pointer of logs_rr_with_thread file
FILE *fplog;

void SwitchThread(int tid)
{
    int i;
    for(i = (tid + 1) % MAX; noOfTimes[i] == 0; i = (i + 1) % MAX)
    {
        // if every thread has finished
        if(i == tid)
        {
            return;
        }
    }

    // switching threads
    turn = i;
}

void * thread_execution(void * pid)
{
	int i = (int)pid;
	int result;
	char line[BUFSIZ];
	int j,cnt2=0;

	FILE *fptr=fopen(p_queue[i].p_name,"r");

	while(noOfTimes[i] != 0)
    {
    	// thread is busy waiting till its turn comes
        while(turn != i);

        if(noOfTimes[i] > time_slice)
        {
        	// will execute till time_slice comes
        	cnt2=time_slice;

			while(cnt2--)
			{
				// print process execution trace in log file
				fprintf(fplog, "\nProcess name: %s\n", p_queue[i].p_name);
				// printf("\nProcess name: %s\n", p_queue[i].p_name);

				// fetch a line from file of process currently under execution
				fgets(line,sizeof(line),fptr);

	        	decodeExec(line,fplog); // calling "decodeExec" function
	        	p_queue[i].insNo++; // updating the number of instructions of process executed

		        // sleep(0.5); // sleep is to simulate the actual running time
			}

	        noOfTimes[i]-=time_slice; // subtracting the number of instructions executed in this loop
        }

        // this thread will have finished after this turn
        else if(noOfTimes[i] > 0 && noOfTimes[i] <= time_slice)
        {
        	while(p_queue[i].insNo!=p_queue[i].cpu_burst_time)
        	{
        		// print process execution trace in log file
				fprintf(fplog, "\nProcess name: %s\n", p_queue[i].p_name);
				// printf("\nProcess name: %s\n", p_queue[i].p_name);

        		// fetch a line from file of process currently under execution
				fgets(line,sizeof(line),fptr);

        		decodeExec(line,fplog); // calling "decodeExec" function
        		p_queue[i].insNo++; // updating the number of instructions of particular file executed

	            // sleep((unsigned int)noOfTimes[i]); // sleep is to simulate the actual running time
        	}
	        noOfTimes[i] = 0; // to terminate the loop
        }
        fprintf(fplog, "\n----------\n");
        SwitchThread(i); // calling SwitchThread function
    }
    pthread_exit(0); // exiting Thread
}

int main()
{
	pcbinitialise();

	fplog=fopen("logs_rr_with_thread","a");
	pthread_t threads[MAX];
	int i, status;

	for(i=0; i<MAX; i++)
	{
		noOfTimes[i] = p_queue[i].cpu_burst_time; // input the burst time of each thread
	}

	for(i=0; i<MAX; i++)
	{
		// create 1 thread for each process
		status=pthread_create(&threads[i], NULL, thread_execution, (void *)i);

		// check if error in thread creation
		if(status!= 0)
		{
			printf("While creating thread %d, pthread_create returned error code %d\n", i, status);
		    exit(-1);
		}
	}

	for(i=0;i<MAX;i++)
	{
		pthread_join(threads[i], 0); // terminate the main program only after all threads terminate
	}

	fprintf(fplog, "\n<--- END of Log file --->");
	fclose(fplog);

	printf("\nLog file 'logs_rr_with_thread' generated!\n");

	printf("here");

	return 0;
}
