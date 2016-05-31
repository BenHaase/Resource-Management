/* Ben Haase
 * CS 4760
 * Assignment 5
 *$Author: o-haase $
 *$Date: 2016/04/12 05:11:30 $
 *$Log: oss.c,v $
 *Revision 1.8  2016/04/12 05:11:30  o-haase
 *cleanup and comments
 *
 *Revision 1.7  2016/04/11 22:33:28  o-haase
 *Additional requests from process fixed
 *
 *Revision 1.6  2016/04/07 01:59:25  o-haase
 *cleanup and comment additions
 *
 *Revision 1.5  2016/04/07 00:24:16  o-haase
 *All features now working properly, needs cleanup
 *
 *Revision 1.4  2016/04/06 20:27:57  o-haase
 *single allocation of max claims working
 *
 *Revision 1.3  2016/04/06 16:45:49  o-haase
 *Added resource allocation and testing for safe state, not currently working correctly
 *
 *Revision 1.2  2016/04/05 19:01:25  o-haase
 *Clock working properly, resource initialization working
 *
 *Revision 1.1  2016/04/03 20:42:10  o-haase
 *Initial revision
 *
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <math.h>
#include <unistd.h>
#include <sys/types.h>
#include <signal.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <semaphore.h>
#include <pthread.h>
#include "rdesc.h"
	
void os(int);
void dda(int);
void sigint_handler(int);
void timeout_handler(int);

rd * rdo; //pointer to set up and access shared memory struct "condition"
int shmid; //memory var for shmat return val
pid_t up[19]; //hold slave process actual pid numbers

int main(int argc, char* argv[]){
	int t;
	int tkill = 60;
	//getopt for time limit if not specied time limit = 60 seconds
	if((t = getopt(argc, argv, "t::")) != -1){
		if(t == 't') tkill = atoi(argv[2]);
		else{
			perror("Error: Options character invalid");
			exit(0);
		}
	}

	srand(time(NULL)); //initialize srand

	//request and format shared memory
	shmid = shmget(SHMKEY, BUFF_SZ, 0777 | IPC_CREAT);
	if(shmid == -1){
		perror("oss: Error in shmget. \n");
		exit(1);
	}
	char * paddr = (char*)(shmat(shmid, 0, 0));
	rdo = (rd*)(paddr);
	rd_init(); //set up shared memory

	os(tkill); //spawn processes

	rd_cleanup(); //cleanup shared memory
	shmctl(shmid, IPC_RMID, (struct shmid_ds*)NULL); //clean up memory
	
	return 0;
}

//function to spawn the process and run the program
void os(int timekill){
	int status; //hold status of finished slave process
	int n = 19; //number of processes
	int nprun = 0; //number of processes running
	int i, j;
	int turn = 1; //indicate starting process in allocation tests
	double avgt = 0; //average time a process is in the system
	int npran = 0; //numer of processes that have ran to completion
	double tc = 500.0; //time constant for logical clock progression (milliseconds)
	double ct = 0; //current time of logical clock
	double waited = 0.0; //time waited by processes
	int procsran[19]; //number of processes ran in each pid slot
	int fail = 0; //incremented if failure, flag for termination
	char t[3]; //hold slave process number
	FILE * otest;
	double tt, wt, cu;
	int lastout = 0; //hold last process to run
	
	//set up signal handlers and real time timer
	signal(SIGINT, sigint_handler);
	signal(SIGALRM, timeout_handler);
	alarm(timekill);

	//initialize resources
	for(i = 0; i < 20; i++){
		rdo->resources[i] = ((rand() % 10) + 1);
		rdo->remaining[i] =  rdo->resources[i];
	}

	//initialize processes ran in pid slots
	for(i = 0; i < 19; i++) procsran[i] = 0;

	//main loop for oss, progress logical clock, spawn processes, run function to allocate resources	
	while(clockr() < 300){

		//if process has ran to completion, remove and extract stats
		for(i = 1; i < 19; i++){
			if(rdo->done[i] > 0){
				lastout = i;
				ct = clockr();
				avgt = (avgt + (ct - (rdo->crd[i].timeEntered)));
				waited = (waited + rdo->crd[i].timeWait);
				procsran[i] = procsran[i] + 1;
				npran++;
				for(j = 0; j < 20; j++){
					rdo->crd[i].has[j] = 0;
					rdo->crd[i].req[j] = 0;
				}
				rdo->reqflg[i] = 0;
				rdo->done[i] = 0;
				rdo->crd[i].status = open;
				rdo->crd[i].timeEntered = 0.0;
				rdo->crd[i].timeWait = 0.0;
				nprun--;
				waitpid(-1, NULL, WNOHANG);
			}	
		}

		//find open slot for new process, if one exists
		i = 1;
		while((i < 20) && (rdo->crd[i].status != open)){ i++;}
		for(j = 1; j < 19; j++) waitpid(-1, NULL, WNOHANG);
		
		//spawn new process
		if((i > 0) && (i < 19) && (nprun < 18)){
			if((up[i] = fork()) < 0){
				perror("Fork failed \n");
				fail = 1;
				return;
			}
			if(up[i] == 0){
				sprintf(t, "%i", i);
				if((execl("./uproc", "uproc", t, NULL)) == -1){
					perror("Exec failed");
					fail = 1;
					return;
				}
			}
			else{
				sem_wait(&(rdo->iniw));
			}
			nprun++;
		}

		//progress logical clock
		clockw(((tc + (rand() % 501)) / 1000.0));
		ct = clockr();

		//allocate resources
		i = turn;
		while(i != (turn - 1)){
			if(rdo->reqflg[i] == 1){
				if(i != lastout){
					dda(i);
				}
			}
			i++;
			i = (i % 19);
		}

		//set new starting point for resource allocation(prevent one process from going repeatedly)
		turn++;
		turn = (turn % 19);
		if(turn == 0){
			turn = (turn + 1);
		}
		
		lastout = 0;
		
		//print clock
		//fprintf(stderr, "clock: %f \n", ct);
	}
	
	//cleanup processes
	fprintf(stderr, "clock: %f \n", clockr());
	fprintf(stderr, "Logical clock timeout \n");
	for(i = 1; i < 19; i++) if(rdo->crd[i].status != open) kill(up[i], SIGQUIT);
	for(i = 1; i < 19; i++) wait();

	tt = (avgt / npran);
	wt = (waited / npran);
	cu = ((tt - wt) / tt);

	//print statistics to screen
	fprintf(stderr, "Throughtput: %i processes \n", npran);
	fprintf(stderr, "Average turnaroud time: %f logical seconds/process \n", tt);
	fprintf(stderr, "Average wait time: %f logical seconds/process\n", wt);
	fprintf(stderr, "CPU utilization: %f \n", cu);

	//print statistics to file
	otest = fopen("osstest", "w");
	fprintf(otest, "Throughtput: %i processes \n", npran);
	fprintf(otest, "Average turnaroud time: %f logical seconds/process \n", tt);
	fprintf(otest, "Average wait time: %f logical seconds/process \n", wt);
	fprintf(otest, "CPU utilization: %f \n", cu);
	fclose(otest);
}

//function to identify candidate for test resource allocation, and to allocate resources
void dda(int t){
	int rem[20]; //remaining resources (available)
	int req[19]; //requests for allocation
	int need[19][20]; //need for each process
	int pal[19][20]; //what is allocated to each process
	int preq[19][20]; //what each process has requested
	int max[19][20]; //max claims for each process
	int rcomp[19]; //if the process is running
	int i, j, r, x, change;
	i = 0;
	j = 0;
	r = 0;
	x = 1;
	change = 1;
	
	//initializing arrays to existing values
	for(i = 0; i < 20; i++){
		rem[i] = rdo->remaining[i];
		if(i < 19){
			req[i] = rdo->reqflg[i];
			if(rdo->crd[i].running == 1){
				rcomp[i] = 1;
			}
			else{
				rcomp[i] = 0;
			}
		}
	}

	//initialized hypothetical info
	for(i = 0; i < 19; i++){
		for(j = 0; j < 20; j++){
			if(rcomp[i] > 0){
				pal[i][j] = rdo->crd[i].has[j];
				preq[i][j] = rdo->crd[i].req[j];
				max[i][j] = rdo->crd[i].max[j];
				need[i][j] = (max[i][j] - pal[i][j]);
			}
			else{
				pal[i][j] = 0;
				preq[i][j] = 0;
				max[i][j] = 0;
				need[i][j] = 0;
			}
		}
	}

	//if not possible to fill, return
	for(i = 0; i < 20; i++){
		if(need[t][i] > rem[i]){
			return;
		}
	}

	//hypothetically allocate
	for(i = 0; i < 20; i++){
		if(rdo->shared[i] < 1){
			rem[i] = (rem[i] - preq[t][i]);
			pal[t][i] = (pal[t][i] + preq[t][i]);
			need[t][i] = (max[t][i] - pal[t][i]);
		}
	}

	//determine if possible allocation could occur, test outcome (for deadlock), allocate (or don't) based on results
	while((x > 0) && (change > 0)){
		change = 0;
		for(i = 1; i < 19; i++){
			if(rcomp[i] == 1){
				r = 0;
				for(j = 0; j < 20; j++){
					if(rem[j] < need[i][j]) r = 1;
				}
				if(r == 0){
					for(j = 0; j < 20; j++){
						if(rdo->shared[j] < 1){
							rem[j] = rem[j] + pal[i][j];
						}
					}
					rcomp[i] = 0;
					i = 19;
					change = 1;
				}
			}
		}
		x = 0;
		for(i = 0; i < 19; i++){
			x = x + rcomp[i];
		}
	}
	
	//if all requests satisfied, allocate the resources
	if(x == 0){
		for(i = 0; i < 20; i++){
			if(rdo->shared[i] < 1){
				rdo->crd[t].has[i] = (rdo->crd[t].has[i] + rdo->crd[t].req[i]);
				rdo->remaining[i] = (rdo->remaining[i] - rdo->crd[t].req[i]);
			}
			rdo->crd[t].req[i] = 0;
		}
		rdo->reqflg[t] = 0;
		fprintf(stderr, "clock: %f \n", clockr());
		fprintf(stderr, "Resources allocated to: %i \n", t);
		sem_post(&(rdo->crd[t].csem));
	}
	
}

//signal handler for SIGINT
void sigint_handler(int s){
	int i;
	printf("\n");
	for(i = 1; i < 19; i++) if(rdo->crd[i].status != open) kill(up[i], SIGTERM);
	for(i = 1; i < 19; i++) wait();
	rd_cleanup();
	shmctl(shmid, IPC_RMID, (struct shmid_ds*)NULL);
	fprintf(stderr, "oss: dying due to interrupt \n");
	exit(1);
}

//signal handler for SIGALRM (timeout)
void timeout_handler(int s){
	int i;
	printf("\n");
	for(i = 1; i < 19; i++) if(rdo->crd[i].status != open) kill(up[i], SIGQUIT);
	for(i = 1; i < 19; i++) wait();
	rd_cleanup();
	shmctl(shmid, IPC_RMID, (struct shmid_ds*)NULL);
	fprintf(stderr, "oss: dying due to timeout \n");
	exit(1);
}
