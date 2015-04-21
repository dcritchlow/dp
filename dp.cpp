#include <iostream>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>
#include "dp.h"
using namespace std;
char outputFlag = ' ';

typedef struct sticks {
	pthread_mutex_t **lock;
	int phil_count;
} Sticks;

void pickup(Phil_struct *ps)
{
	Sticks *pp;
//	int i;
	int phil_count;

	pp = (Sticks *) ps->v;
	phil_count = pp->phil_count;

	if(!pthread_mutex_lock(pp->lock[ps->id])) {
		printf("#%d left chopstick not available\n", ps->id);
//		int milisec = 100; // length of time to sleep, in miliseconds
		struct timespec req;
		req.tv_sec = 0;
		req.tv_nsec = 100000000ULL * rand() / RAND_MAX;
		nanosleep(&req, (struct timespec *)NULL);
		printf("#%d sleeping for %d ns\n", ps->id, req.tv_nsec);
	}
	if(!pthread_mutex_lock(pp->lock[(ps->id+1)%phil_count])) {
		printf("#%d right chopstick not available\n", ps->id);
//		int milisec = 100; // length of time to sleep, in miliseconds
		struct timespec req;
		req.tv_sec = 0;
		req.tv_nsec = 100000000ULL * rand() / RAND_MAX;
		nanosleep(&req, (struct timespec *)NULL);
		printf("#%d sleeping for %d ns\n", ps->id, req.tv_nsec);
	}
}

void putdown(Phil_struct *ps)
{
	Sticks *pp;
//	int i;
	int phil_count;

	pp = (Sticks *) ps->v;
	phil_count = pp->phil_count;

	pthread_mutex_unlock(pp->lock[(ps->id+1)%phil_count]); /* unlock right stick */
	pthread_mutex_unlock(pp->lock[ps->id]);  /* unlock left stick */
}

void *initialize_v(int phil_count)
{
	Sticks *pp;
	int i;

	pp = (Sticks *) malloc(sizeof(Sticks));

	pp->phil_count = phil_count;
	pp->lock = (pthread_mutex_t **) malloc(sizeof(pthread_mutex_t *)*phil_count);
	if (pp->lock == NULL) { perror("malloc"); exit(1); }
	for (i = 0; i < phil_count; i++) {
		pp->lock[i] = (pthread_mutex_t *) malloc(sizeof(pthread_mutex_t));
		if (pp->lock[i] == NULL) { perror("malloc"); exit(1); }
	}
	for (i = 0; i < phil_count; i++) {
		pthread_mutex_init(pp->lock[i], NULL);
	}

	return (void *) pp;
}

void *philosopher(void *v)
{
	Phil_struct *ps;
	int st;
	int t;

	ps = (Phil_struct *) v;

	while(1) {

		/* First the philosopher thinks for a random number of seconds */

		st = (random()%ps->ms);
		printf("#%d thinking for %d second%s\n",
			   ps->id, st, (st == 1) ? "" : "s");
		ps->thinktime[ps->id]+= st;
		fflush(stdout);
		sleep(st);

		/* Now, the philosopher wakes up and wants to eat.  He calls pickup
           to pick up the chopsticks */

		printf("#%d waiting for mutex\n",
			   ps->id);
		fflush(stdout);
		t = time(0);
		pthread_mutex_lock(ps->waitmon);
		ps->blockstarting[ps->id] = t;
		pthread_mutex_unlock(ps->waitmon);

		pickup(ps);

		pthread_mutex_lock(ps->waitmon);
		ps->waittime[ps->id] += (time(0) - t);
		ps->blockstarting[ps->id] = -1;
		pthread_mutex_unlock(ps->waitmon);

		/* When pickup returns, the philosopher can eat for a random number of
           seconds */

		st = (random()%2);
		printf("#%d eating for %d second%s\n",
			   ps->id, st, (st == 1) ? "" : "s");
		ps->mealcount[ps->id] += 1;
		ps->mealtime[ps->id] += st;
		fflush(stdout);
		sleep(st);

		/* Finally, the philosopher is done eating, and calls putdown to
           put down the chopsticks */

		printf("#%d releasing chopsticks\n",
			   ps->id);

		fflush(stdout);
		putdown(ps);
	}
}

int main(int argc, char **argv) {
	int i;
	pthread_t *threads;
	Phil_struct *ps;
	void *v;
	int t0, ttmp, ttmp2;
	pthread_mutex_t *waitmon;
	int *waittime;
	int *eatingtime;
	int *mealcount;
	int *thinktime;
	int *mealtime;
	int *blockstarting;
	int phil_count;
	int waiting;
	int thinks;
	int runseconds;
	int ran = 0;
	double averagemeals =0;
	double averagemealtime =0;
	double averagewaittime =0;
	double averagethinktime =0;

	if (argc != 3) {
		fprintf(stderr, "usage: dp philosopher_count maxsleepsec\n");
		exit(1);
	}

	srandom(time(0));

	phil_count = atoi(argv[1]);
	runseconds = atoi(argv[2]);
	threads = (pthread_t *) malloc(sizeof(pthread_t)*phil_count);
	if (threads == NULL) { perror("malloc"); exit(1); }
	ps = (Phil_struct *) malloc(sizeof(Phil_struct)*phil_count);
	if (ps == NULL) { perror("malloc"); exit(1); }
	v = initialize_v(phil_count);
	t0 = time(0);
	waittime = (int *) malloc(sizeof(int)*phil_count);
	if (waittime == NULL) { perror("malloc waittime"); exit(1); }
	eatingtime = (int *) malloc(sizeof(int)*phil_count);
	if (eatingtime == NULL) { perror("malloc eatingtime"); exit(1); }
	mealcount = (int *) malloc(sizeof(int)*phil_count);
	if (mealcount == NULL) { perror("malloc mealcount"); exit(1); }
	thinktime = (int *) malloc(sizeof(int)*phil_count);
	if (thinktime == NULL) { perror("malloc thinktime"); exit(1); }
	mealtime = (int *) malloc(sizeof(int)*phil_count);
	if (mealtime == NULL) { perror("malloc mealtime"); exit(1); }
	blockstarting = (int *) malloc(sizeof(int)*phil_count);
	if (blockstarting == NULL) { perror("malloc blockstarting"); exit(1); }

	waitmon = (pthread_mutex_t *) malloc(sizeof(pthread_mutex_t));
	pthread_mutex_init(waitmon, NULL);
	for (i = 0; i < phil_count; i++) {
		waittime[i] = 0;
		eatingtime[i] = 0;
		mealcount[i] = 0;
		thinktime[i] = 0;
		blockstarting[i] = -1;
	}

	for (i = 0; i < phil_count; i++) {
		ps[i].id = i;
		ps[i].t0 = t0;
		ps[i].v = v;
		ps[i].ms = 4;
		ps[i].waittime = waittime;
		ps[i].eatingtime = eatingtime;
		ps[i].mealcount = mealcount;
		ps[i].mealtime = mealtime;
		ps[i].thinktime = thinktime;
		ps[i].blockstarting = blockstarting;
		ps[i].waitmon = waitmon;
		ps[i].phil_count = phil_count;
		pthread_create(threads+i, NULL, philosopher, (void *) (ps+i));
	}

	while(1) {
		if (ran == 0)
			printf("Timer running for %d seconds\n", runseconds);
		pthread_mutex_lock(waitmon);
		ttmp = time(0);
		waiting = 0;
		thinks = 0;
		for(i=0; i < phil_count; i++) {
			waiting += waittime[i];
			thinks += thinktime[i];
			if (blockstarting[i] != -1){
				waiting += (ttmp - blockstarting[i]);
				waittime[i] += (ttmp - blockstarting[i]);
			}
		}

		for(i=0; i < phil_count; i++) {
			ttmp2 = waittime[i];
			if (blockstarting[i] != -1) ttmp2 += (ttmp - blockstarting[i]);
			ran += 1;

		}

		pthread_mutex_unlock(waitmon);
		fflush(stdout);
		if(ran > phil_count) {
			cout << "Time's up" << endl;
			for(i=0; i<phil_count; i++){
				cout << "#" << i << " MealCount: " << mealcount[i]
				<< " MealTime: " << mealtime[i]
				<< " WaitTime: " << waittime[i]
				<< " ThinkTime: " << thinktime[i]
				<< endl;
				averagemeals += mealcount[i];
				averagemealtime += mealtime[i];
				averagewaittime += waittime[i];
				averagethinktime += thinktime[i];
			}
			cout << "Averages:"
			<< " MealCount: "
			<< (double)averagemeals / phil_count
			<< " MealTime: "
			<< (double)averagemealtime / phil_count
			<< " WaitTime: "
			<< (double)averagewaittime / phil_count
			<< " ThinkTime: "
			<< (double)averagethinktime / phil_count
			<< endl;
			exit(0);
		}
		sleep(runseconds);

	}
}
