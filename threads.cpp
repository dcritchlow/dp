#include <iostream>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>
using namespace std;
char outputFlag = ' ';

void *myThreadFunction(void *val)
{
    struct random_data buf;
    char statebuf[4096];
    unsigned int seed;
    struct timespec nanotime;
    int32_t seconds;
    int rc;
    
	long threadNumber = (long) val;
    seed = time(NULL) << threadNumber;		// shift the seed threadNumber bits to the left
    rc = initstate_r(seed, statebuf, sizeof(statebuf), &buf);
	if (rc) {
		perror("initstate_r failed");
    	pthread_exit(NULL);
	}

	/* sleep for a random amount of time */
	rc = random_r(&buf, &seconds);
	if (rc) {
		perror("random_r failed");
    	pthread_exit(NULL);
	}
	nanotime.tv_sec = (seconds % 13);		// set sleep seconds from 0 to 13 seconds
	nanotime.tv_nsec = 0;
	if (outputFlag == 'c') {
		cout << "Thread number " << threadNumber 
			<< " sleeping for " << nanotime.tv_sec << " seconds" << endl;
	} else {
		printf("Thread number %ld sleeping for %ld seconds\n", threadNumber, nanotime.tv_sec);
	}
	nanosleep(&nanotime, NULL);			// snooze
	printf("Thread number %ld did sleep for %ld seconds\n", threadNumber, nanotime.tv_sec);
    pthread_exit(NULL);
}

#define COUNT 3

int main(int argc, char **argv) {
    long i, rc;
    pthread_t threads[3];
	if (argc == 2)
		outputFlag = argv[1][0];
    
    // create COUNT number of threads
    for (i = 0; i < COUNT; i++) {
       	rc = pthread_create(&threads[i], NULL, myThreadFunction, (void *)i);
		if (rc) {
        	perror("pthread_create");
        	return 1;
		}
    }
    
    // wait for each thread to exit
    for (i = 0; i < COUNT; i++) {
       	pthread_join(threads[i], NULL);
    }
 
    return 0;
}
