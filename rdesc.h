/* Ben Haase
 * CS 4760
 * Assignment 5
 * $Author: o-haase $
 * $Date: 2016/04/11 22:33:28 $
 * $Log: rdesc.h,v $
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
 * added new array in struct
 *
 * Revision 1.3  2016/04/06 16:45:49  o-haase
 * Added resource allocation and testing for safe state, not currently working correctly
 *
 * Revision 1.2  2016/04/05 19:02:55  o-haase
 * Minor modifications to struct and function prototypes
 *
 * Revision 1.1  2016/04/03 20:44:05  o-haase
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

//key and size for shared memory
#define SHMKEY 56
#define BUFF_SZ sizeof(rd)

typedef enum {running, waiting, done, open} rflag;
//child process struct to hold resource info, semaphores and info
typedef struct{
	int req[20];
	int has[20];
	int max[20];
	double timeEntered;
	double timeWait;
	rflag status;
	sem_t csem;
	int semcount;
	int running;
}child;

//os resource descriptor and logical clock struct
typedef struct{
	int resources[20];
	int remaining[20];
	int shared[20];
	int done[19];
	child crd[19];
	double lclock;
	sem_t cls;
	sem_t iniw;
	int reqflg[19];
}rd;

//function prototypes
	void rd_init();
	double clockr();
	void clockw(double);
	void rd_cleanup();
	
