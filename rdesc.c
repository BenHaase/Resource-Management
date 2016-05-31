/* Ben Haase
 * CS 4760
 * Assignment 5
 * $Author: o-haase $
 * $Date: 2016/04/11 22:33:28 $
 * $Log: rdesc.c,v $
 * Revision 1.7  2016/04/11 22:33:28  o-haase
 * minor changes
 *
 * Revision 1.6  2016/04/07 01:59:25  o-haase
 * cleanup and comment additions
 *
 * Revision 1.5  2016/04/07 00:24:16  o-haase
 * minor changes
 *
 * Revision 1.4  2016/04/06 20:27:57  o-haase
 * added initialization of new array in struct
 *
 * Revision 1.3  2016/04/06 16:45:49  o-haase
 * Added resource allocation and testing for safe state, not currently working correctly
 *
 * Revision 1.2  2016/04/05 19:02:27  o-haase
 * Added functions to control clock access using semaphore
 *
 * Revision 1.1  2016/04/03 20:43:13  o-haase
 * Initial revision
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

	extern rd * rdo;

	//function to initialize struct values and semaphores in shared memory
	void rd_init(){
		int i, j;
		for(i = 0; i < 19; i++){
			rdo->crd[i].timeEntered = 0.0;
			rdo->crd[i].status = open;
			for(j = 0; j < 20; j++){
				rdo->crd[i].req[j] = 0;
				rdo->crd[i].has[j] = 0;
				rdo->crd[i].max[j] = 0;
			}
			rdo->reqflg[i] = 0;
			rdo->done[i] = 0;
			rdo->crd[i].timeWait = 0.0;
			rdo->crd[i].semcount = 0;
			rdo->crd[i].running = 0;
		}
		for(i = 0; i < 20; i++){
			rdo->resources[i] = (rand() % 10) + 1;
			rdo->remaining[i] = rdo->resources[i];
			rdo->shared[i] = 0;
		}
		rdo->lclock = 0;
		j = (rand() % 3) + 3;
		for(i = 0; i < j; i++){
			rdo->shared[i] = 1;
		}
		if((sem_init(&(rdo->cls), 1, 1)) < 0) perror("Semaphore cls initialization error");
		if((sem_init(&(rdo->iniw), 1, 0)) < 0) perror("Semaphore iniw initialization error");
		for(i = 0; i < 19; i++){
			if((sem_init(&(rdo->crd[i].csem), 1, 0)) < 0) perror("Semaphore csem %i initialization error");
		}
	}

	//semaphore protected clock read function
	double clockr(){
		double t;
		sem_wait(&(rdo->cls));
		t = rdo->lclock;
		sem_post(&(rdo->cls));
		return t;
			
	}

	//semaphore protected clock write function
	void clockw(double a){
		sem_wait(&(rdo->cls));
		(rdo->lclock) = (rdo->lclock) + a;
		sem_post(&(rdo->cls));
	}

	//semaphore cleanup function for termination
	void rd_cleanup(){
		int i;
		if((sem_destroy(&(rdo->cls))) < 0) perror("Semaphore cls destruction error");
		if((sem_destroy(&(rdo->iniw))) < 0) perror("Semaphore iniw destruction error");
		for(i = 0; i < 19; i++){
			if((sem_destroy(&(rdo->crd[i].csem))) < 0) perror("Semaphore csem %i initialization error");
		}	
	}
