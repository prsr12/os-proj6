#include <stdio.h>
#include <unistd.h>
#include <semaphore.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include <signal.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <pthread.h>
#include <semaphore.h>
#include <errno.h>
#include <stdbool.h>
#include <sys/shm.h>
#include <sys/ipc.h>
#include <sys/types.h>
#include <time.h>
#include <stdlib.h>
#include <errno.h>
#include <limits.h>
#include <sys/msg.h>
#include <string.h>
#include <sys/wait.h>
#define structsize ((sizeof(array_size)/sizeof(array_size[0])) * 18)
#define sem_size sizeof(int)
#define que_size 18
#ifndef FALSE
#define FALSE (0)
#endif
#ifndef TRUE
#define TRUE (!FALSE)
#endif

typedef struct memory_manager{
        pid_t pid;
        int resource_Marker;
        int tablesize[32];

} memory_manager;

struct memory_manager *array_size[18];
struct memory_manager *(*ArrayPointer)[] = &array_size;

pid_t pid = 0;
extern int limit;
extern int percentage;

pid_t forkChild(char *sharedTimeMem, char *sharedSemMem, char*sharedPositionMem, char*ShrdMem, char*sharedLimitMem, char*sharedPercentageMem){
        if((pid = fork()) == 0){
                execlp("./worker", "./worker", sharedTimeMem, sharedSemMem, sharedPositionMem, ShrdMem, sharedLimitMem, sharedPercentageMem, NULL);
        }
        if(pid < 0){
                printf("Fork Error %s\n", strerror(errno));
        }
        return pid;
};

void makeShMem(int *timeid, int *semid, int*ID, key_t timeKey, key_t semKey, key_t Key){

       *timeid = shmget(timeKey, (sizeof(unsigned int) * 2), 0666 | IPC_CREAT);
        *semid = shmget(semKey, (sizeof(unsigned int) * 2), 0666 | IPC_CREAT);
        *ID = shmget(Key, (sizeof(memory_manager) *2),0666|IPC_CREAT);

        if(*timeid == -1){
                printf("Timed ID: %s\n", strerror(errno));
        }
        if(*semid == -1){
                printf("Semaphore ID: %s\n", strerror(errno));
        }
        if(*ID == -1){
                printf("Resource ID:  %s\n", strerror(errno));
        }
};

void makeShMemKey(key_t *Key, key_t *semKey, key_t *timeKey) {
        *Key = ftok(".", 4328);
        *semKey = ftok(".", 8993);
        *timeKey = ftok(".", 2820);
};

void rsg_manage_args(char *sharedTimeMem, char *sharedSemMem, char*sharedPositionMem, char*ShrdMem, char*sharedLimitMem, char*sharedPercentageMem, int timeid, int semid, int ID, int placementMarker, int limit, int percentage){
        snprintf(sharedTimeMem, sizeof(sharedTimeMem)+25, "%d", timeid);
        snprintf(sharedSemMem, sizeof(sharedSemMem)+25, "%d", semid);
        snprintf(sharedPositionMem, sizeof(sharedPositionMem)+25, "%d", placementMarker);
        snprintf(ShrdMem, sizeof(ShrdMem)+25, "%d", ID);
        snprintf(sharedLimitMem, sizeof(sharedLimitMem)+25, "%d", limit);
        snprintf(sharedPercentageMem, sizeof(sharedPercentageMem)+25, "%d", percentage);
};

void ShMemAttach(unsigned int **seconds, unsigned int **nanoseconds, sem_t **semaphore, memory_manager **Pointer, int timeid, int semid, int ID){
        *seconds = (unsigned int*)shmat(timeid, NULL, 0);
        if(**seconds == -1){
                printf("Seconds %s\n", strerror(errno));
        }
        *nanoseconds = *seconds + 1;
        if(**nanoseconds == -1){
                printf("Nanoseconds %s\n", strerror(errno));
        }
        *semaphore = (sem_t*)shmat(semid, NULL, 0);
        if(*semaphore == (void*)-1){
                printf("Semaphore %s\n", strerror(errno));
        }
        *Pointer = (memory_manager*)shmat(ID, NULL, 0);
        if(*Pointer == (void*)-1){
                printf("Resource %s\n", strerror(errno));
        }
};
