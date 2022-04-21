/*	
	Autor:		Martin Hlinský
	Login:		xhlins01
	Datum:		06.05.2020
	Projekt:	IOS #2 - Faneuil Hall Problem
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include <time.h>

#include <semaphore.h>
#include <pthread.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/mman.h>

#define MMAP(ptr) {(ptr) = mmap(NULL, sizeof(*(ptr)), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);}
#define UNMAP(ptr) {munmap((ptr), sizeof((ptr)));}

FILE *file;

pid_t court, generator, IID, wpid;

/*semaphores*/
/*
noJudge = can immigrants enter/leave?
mutex = primarily for organized check
confirmed = only a certain amount of immigrants confirmed
allRegistered = entered == check?
msg = organized output, only one at a time
confirmedDone = no interrupting judge
*/
sem_t *noJudge = NULL;
sem_t *mutex = NULL;
sem_t *confirmed = NULL;
sem_t *allRegistered = NULL;
sem_t *msg = NULL;
sem_t *confirmedDone = NULL;

/*shared vars*/
int *action = NULL;
int *entered = NULL;
int *registered = NULL;
int *immCount = NULL;
int *judge = NULL;
int *done = NULL;
int *status = NULL;

/*arg check*/
int argCheck(int PI, int IG, int JG, int IT, int JT){
	if (PI >= 1) {
		
	}
	else {
		fprintf(stderr,"PI must be greater or equal to 1\n"); /*error*/
		return 1;
	}

	if ((IG >= 0) && (IG <= 2000)) {
		
	}
	else {
		fprintf(stderr,"IG must be between 0 and 2000\n"); /*error*/
		return 1;
	}

	if ((JG >= 0) && (JG <= 2000)) {
		
	}
	else {
		fprintf(stderr,"JG must be between 0 and 2000\n"); /*error*/
		return 1;
	}

	if ((IT >= 0) && (IT <= 2000)) {
		
	}
	else {
		fprintf(stderr,"IT must be between 0 and 2000\n"); /*error*/
		return 1;
	}

	if ((JT >= 0) && (JT <= 2000)) {
		
	}
	else {
		fprintf(stderr,"JT must be between 0 and 2000\n"); /*error*/
		return 1;
	}
	return 0;
}

/*init function*/
int init() {
	/*open file*/
	file = fopen("proj2.out", "w");

	/*mmap shared vars*/
	MMAP(action);
	MMAP(entered);
	MMAP(registered);
	MMAP(immCount);
	MMAP(judge);
	MMAP(done);
	MMAP(status);

	/*open semaphores*/
	if ((noJudge = sem_open("/xhlins01.ios.proj2.noJudge", O_CREAT | O_EXCL, 0666, 1)) == SEM_FAILED) {
		return 1;
	}
	if ((mutex = sem_open("/xhlins01.ios.proj2.mutex", O_CREAT | O_EXCL, 0666, 1)) == SEM_FAILED) {
		return 1;
	}
	if ((msg = sem_open("/xhlins01.ios.proj2.msg", O_CREAT | O_EXCL, 0666, 1)) == SEM_FAILED) {
		return 1;
	}
	if ((confirmed = sem_open("/xhlins01.ios.proj2.confirmed", O_CREAT | O_EXCL, 0666, 0)) == SEM_FAILED) {
		return 1;
	}
	if ((allRegistered = sem_open("/xhlins01.ios.proj2.allRegistered", O_CREAT | O_EXCL, 0666, 0)) == SEM_FAILED) {
		return 1;
	}
	if ((confirmedDone = sem_open("/xhlins01.ios.proj2.confirmedDone", O_CREAT | O_EXCL, 0666, 1)) == SEM_FAILED) {
		return 1;
	}
	return 0;
}

/*immigrant process*/
void process_immigrant(int i, int IT) {
	
	sem_wait(msg);
	(*action)++;
	fprintf(file, "%d      : IMM %d      : starts\n", *action, i);
	fflush(file);
	sem_post(msg);
		
	sem_wait(noJudge);
	
	sem_wait(msg);
	(*action)++;
	(*entered)++;
	(*immCount)++;
	fprintf(file, "%d      : IMM %d      : enters      : %d      : %d      : %d\n", *action, i, *entered, *registered, *immCount);
	fflush(file);
	sem_post(msg);
		
	sem_post(noJudge); 
	
	sem_wait(mutex);

	sem_wait(msg);
	(*action)++;
	(*registered)++;
	fprintf(file, "%d      : IMM %d      : checks      : %d      : %d      : %d\n", *action, i, *entered, *registered, *immCount);
	fflush(file);
	sem_post(msg);

	if ((*judge == 1) && (*entered == *registered)) {
		sem_post(allRegistered);
		sem_post(mutex);
	}
	else {
		sem_post(mutex);
	}
	
	sem_wait(confirmed);

	sem_wait(confirmedDone);
	sem_post(confirmedDone);

	sem_wait(msg);
	(*action)++;
	fprintf(file, "%d      : IMM %d      : wants certificate      : %d      : %d      : %d\n", *action, i, *entered, *registered, *immCount);
	fflush(file);
	sem_post(msg);

	if (IT > 0) {
		usleep((rand() % (IT + 1)) * 1000);
	}
	
	sem_wait(confirmedDone);
	sem_post(confirmedDone);

	sem_wait(msg);
	(*action)++;
	fprintf(file, "%d      : IMM %d      : got certificate      : %d      : %d      : %d\n", *action, i, *entered, *registered, *immCount);
	fflush(file);
	sem_post(msg);
	
	sem_wait(noJudge);

	sem_wait(msg);
	(*action)++;
	(*immCount)--;
	fprintf(file, "%d      : IMM %d      : leaves      : %d      : %d      : %d\n", *action, i, *entered, *registered, *immCount);
	fflush(file);
	sem_post(msg);

	sem_post(noJudge);
		
	exit(0);
}

/*immigrant generator*/
void create_immigrants(int PI, int IG, int IT) {
	*status = 0;
	for (int i = 1; i <= PI; i++) {
		if (IG > 0) {
			usleep((rand() % (IG + 1)) * 1000);
		}
		
		pid_t IID = fork();
		if (IID == 0) {
			process_immigrant(i, IT);
		}
		else if (IID < 0) {
			exit(1);
		}
	}

	while ((wpid = wait(status)) > 0);
	exit(0);
}

/*judge process*/
void process_judge(int JG, int PI, int JT) {
	*done = 0;
	while (PI > *done) {
		if (JG > 0) {
			usleep((rand() % (JG + 1)) * 1000);
		}
				
		sem_wait(noJudge);
		sem_wait(mutex);

		sem_wait(msg);
		(*action)++;
		fprintf(file, "%d      : JUDGE      : wants to enter\n", *action);
		fflush(file);
		sem_post(msg);

		sem_wait(msg);
		(*action)++;
		fprintf(file, "%d      : JUDGE      : enters      : %d      : %d      : %d\n", *action, *entered, *registered, *immCount);
		fflush(file);
		sem_post(msg);
				
	
		*judge = 1;
		if (*entered > *registered) {
			sem_wait(msg);
			(*action)++;
			fprintf(file, "%d      : JUDGE      : waits for imm      : %d      : %d      : %d\n", *action, *entered, *registered, *immCount);
			fflush(file);
			sem_post(msg);
			sem_post(mutex);
			sem_wait(allRegistered);
			sem_wait(mutex);
		}

		sem_wait(confirmedDone);

		sem_wait(msg);
		(*action)++;
		fprintf(file, "%d      : JUDGE      : starts confirmation      : %d      : %d      : %d\n", *action, *entered, *registered, *immCount);
		fflush(file);
		sem_post(msg);

		
		
		for (int i = 0; i < *registered; i++) {
			if (JT > 0) {
				usleep((rand() % (JG + 1)) * 1000);
			}
			sem_post(confirmed);
			(*done)++;
		}

		sem_wait(msg);
		*entered = 0;
		*registered = 0;
		(*action)++;
		fprintf(file, "%d      : JUDGE      : ends confirmation      : %d      : %d      : %d\n", *action, *entered, *registered, *immCount);
		fflush(file);
		sem_post(msg);
		
		sem_post(confirmedDone);
		
		sem_wait(msg);
		(*action)++;
		fprintf(file, "%d      : JUDGE      : leaves      : %d      : %d      : %d\n", *action, *entered, *registered, *immCount);
		fflush(file);
		sem_post(msg);

		*judge = 0;
		if (PI == *done) {
			sem_wait(msg);
			(*action)++;
			fprintf(file, "%d      : JUDGE      : finishes\n", *action); 
			fflush(file);
			sem_post(msg);			
			sem_post(noJudge); 
			sem_post(mutex);
			exit(0);
		}

		sem_post(noJudge); 
		sem_post(mutex);
		
	}
	exit(0);
}

void cleanup() {
	/*unmap shared vars*/
	UNMAP(action);
	UNMAP(entered);
	UNMAP(registered);
	UNMAP(immCount);
	UNMAP(judge);
	UNMAP(done);
	UNMAP(status);
	
	/*close semaphores*/
	sem_close(noJudge);
	sem_close(mutex);
	sem_close(confirmed);
	sem_close(allRegistered);
	sem_close(msg);
	sem_close(confirmedDone);

	/*unlink semaphores*/
	sem_unlink("/xhlins01.ios.proj2.noJudge");
	sem_unlink("/xhlins01.ios.proj2.mutex");
	sem_unlink("/xhlins01.ios.proj2.confirmed");
	sem_unlink("/xhlins01.ios.proj2.allRegistered");
	sem_unlink("/xhlins01.ios.proj2.msg");
	sem_unlink("/xhlins01.ios.proj2.confirmedDone");

	/*close file*/
	if (file != NULL) {
		fclose(file);
	}
}

int main(int argc, char* argv[]) {

	/*randomized seed*/
	srand(time(0));
	//setbuf(file, NULL);

	/*6 arguments needed*/
	if (argc != 6) {
		fprintf(stderr, "wrong amount of arguments\n");
		return 1;
	}

	/*Convert chars to int*/
	int PI = atoi(argv[1]);
	int IG = atoi(argv[2]);
	int JG = atoi(argv[3]);
	int IT = atoi(argv[4]);
	int JT = atoi(argv[5]);

	/*argcheck*/
	if (argCheck(PI,IG,JG,IT,JT) == 1) {
		return 1;
	}

	/*if init fails, else continue*/
	if (init() == 1) {
		cleanup();
		return 1;
	}
	setbuf(file, NULL);
	/*process that runs judge*/
	pid_t court = fork();
	if (court < 0) {
		/*error*/
		return 1;
	}
	else if (court == 0) {
		/*child*/
		process_judge(JG, PI, JT);
	}

	/*process generating immigrants*/
	pid_t generator = fork();
	if (generator < 0) {
		return 1;
	}
	else if (generator == 0) {
		create_immigrants(PI, IG, IT);
	}

	/*wait for the other processes*/
	waitpid(generator, NULL, 0);
	waitpid(court, NULL, 0);
	
	/*cleanup everything*/
	cleanup();
	return 0;
}
