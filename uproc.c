/* Ben Haase
 * CS 4760
 * Assignment 5
 *$Author: o-haase $
 *$Date: 2016/04/12 05:11:30 $
 *$Log: uproc.c,v $
 *Revision 1.8  2016/04/12 05:11:30  o-haase
 *cleanup and comments
 *
 *Revision 1.7  2016/04/11 22:33:28  o-haase
 *waits for additional memory request to be filled
 *
 *Revision 1.6  2016/04/07 01:59:25  o-haase
 *cleanup and comment additions
 *
 *Revision 1.5  2016/04/07 00:24:16  o-haase
 *Processes now randomly request after initial request, max claims implemented, all features working
 *
 *Revision 1.4  2016/04/06 20:27:57  o-haase
 *working with single max allocation
 *
 *Revision 1.3  2016/04/06 16:45:49  o-haase
 *Added resource allocation and testing for safe state, not currently working correctly
 *
 *Revision 1.2  2016/04/05 19:03:26  o-haase
 *Added resource requests, working correctly
 *
 *Revision 1.1  2016/04/03 20:44:37  o-haase
 *Initial revision
 *
 */
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <semaphore.h>
#include <pthread.h>
#include "rdesc.h"

void pmsg(char*, char*);
void sigtimeout_handler(int);
void sigctlc_handler(int);

//globals for use in signal handlers
char * paddr; //memory address returned from shmat
int pn; //integer representation of process number (from master)
char * pnc; //character representation of process number (from master)
rd * rdo;  //format and reference shared memory struct

int main(int argc, char* argv[]){
	//setup signal handlers (ignore SIGINT that will be intercepted by master)
	//signal(SIGINT, SIG_IGN);
	signal(SIGQUIT, sigtimeout_handler);
	signal(SIGTERM, sigctlc_handler);
	signal(SIGINT, SIG_IGN);

	//set process number and char representation
	pn = atoi(argv[1]);
	pnc = argv[1];
	srand((unsigned)(time(NULL) + pn)); //initialize srand

	//mount shared memory
	int shmid = shmget(SHMKEY, BUFF_SZ, 0777);
	if(shmid == -1){
		perror("User process: Error in shmget opening.");
		exit(1);
	}
	paddr = (char*)(shmat(shmid, 0, 0));
	rdo = (rd*)(paddr);

	//set time process entered the system and its status
	rdo->crd[pn].timeEntered = clockr();
	rdo->crd[pn].status = running;
	rdo->crd[pn].running = 1;

	int x;
	int i = 0;
	int j = 0;
	int exit = 0; //determine if process should exit loop
	//hold clock values
	double cl = 0.0;
	double clsub = 0.0;
	double clx = 0.0;

	//determine max claims and initial requests
	for(i = 0; i < 20; i++){
		rdo->crd[pn].max[i] = rand() % ((rdo->resources[i] + 1) - (rdo->crd[pn].has[i]));
		rdo->crd[pn].req[i] = rand() % ((rdo->crd[pn].max[i] + 1) - (rdo->crd[pn].has[i]));
	}
	
	fprintf(stderr, "clock: %f \n", clockr());
	fprintf(stderr, "Process %i: requesting initial resources \n", pn);

	//set status and wait for allocation to run
	rdo->reqflg[pn] = 1;
	rdo->crd[pn].status = waiting;
	sem_post(&(rdo->iniw));
	clx = clockr();
	rdo->crd[pn].semcount = 1;
	sem_wait(&(rdo->crd[pn].csem));
	rdo->crd[pn].timeWait = clockr() - rdo->crd[pn].timeEntered;

	//run continuously updating logical clock and requesting resources until exit is set
	while(exit < 1){
		cl = clockr();
		x = (rand() % 5) + 2;
		rdo->crd[pn].status = running;
		for(j = 0; j < x; j++){
			clockw((10.0 / 1000.0));
			clsub = clockr();
			if((clsub - clx) > 0.25){
				clx = clockr();
				if((clsub - (rdo->crd[pn].timeEntered)) > 1.0){
					exit = rand() % 2;
				}
				if(exit < 1){
					for(i = 0; i < 20; i++){
						if((rdo->crd[pn].has[i] < rdo->crd[pn].max[i])){
							rdo->crd[pn].req[i] = rand() % ((rdo->crd[pn].max[i]) - (rdo->crd[pn].has[i]));
						}
					}
					fprintf(stderr, "clock: %f \n", clockr());
					fprintf(stderr, "Process %i: requesting additional resources \n", pn);
					rdo->reqflg[pn] = 1;
					sem_wait(&(rdo->crd[pn].csem));
				}
			}
			if(exit > 0){
				j = x;
			}
		}
	}

	fprintf(stderr, "clock: %f \n", clockr());
	fprintf(stderr, "%i is done resources being released \n", pn);
	//release resources
	for(i = 0; i < 20; i++){
		if(rdo->shared[i] < 1){
			rdo->remaining[i] += rdo->crd[pn].has[i];
		}
	}

	//set status to done for resource and statistic extraction
	rdo->crd[pn].status = done;
	rdo->crd[pn].running = 0;
	rdo->done[pn] = 1;

	//dismount memory and print finished message
	shmdt(paddr);
	return 0;
}

//function to print process messages with time
void pmsg(char *p, char *msg){
	time_t tm;
	char * tms;
	time(&tm);
	tms = ctime(&tm);
	fprintf(stderr, "Process: %02s %s at %.8s.\n", p, msg, (tms + 11));
}

//signal handler for SIGQUIT (sent from master on timeout)
void sigtimeout_handler(int s){
	shmdt(paddr);
	fprintf(stderr, "User process: %02s dying due to timeout \n", pnc);
	exit(1);
}

//signal handle for SIGTERM (sent from master on ^C)
void sigctlc_handler(int s){
	shmdt(paddr);
	fprintf(stderr, "User process: %02s dying due to interrupt \n", pnc);
	exit(1);
}
