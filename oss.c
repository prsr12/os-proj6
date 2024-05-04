#include "header.h"
#define SECOND_TIMER 100

int alrm, processCount = 18, frameTablePos = 0;
int setArr[18] = {0};

struct memory_resource {
    long msgString;
    char msgChar[100];
} message;

void timekill(int sign_no){
    alrm = 1;
}

int arrayChecker(int *placementMarker){
    int inc = 0;
    for(inc = 0; inc < processCount; inc++){
        if(setArr[inc] == 0){
            setArr[inc] = 1;
            *placementMarker = inc;
            return 1;
        }
    }
    return 0;
}

void timeToFork(unsigned int *seconds, unsigned int *nanoseconds, unsigned int *forkTimeSeconds, unsigned int *forkTimeNanoseconds){
    unsigned int random = rand()%500000000;
    *forkTimeNanoseconds = 0;
    *forkTimeSeconds = 0;
    if((random + *nanoseconds) >= 1000000000){
        *forkTimeSeconds += 1;
        *forkTimeNanoseconds = (random + *nanoseconds) - 1000000000;
    } else {
        *forkTimeNanoseconds = random + *nanoseconds;
    }
    *forkTimeSeconds = *seconds;
}

int main (int argc, char *argv[]) {

    int percentage = 50;
    int maxProcL = 100;

    char childMsg[20], ch;
    char requestType[20];
    char sharedPositionMem[10];
    char sharedPercentageMem[10];
    char sharedTimeMem[10];
    char rscShrdMem[10];
    char sharedSemMem[10];
    char sharedLimitMem[10];

    int address;
    int forked = 0;
    int lines = 0;
    int frameLoop = 0;
    int pagefault = 0;
    int requests = 0;
    int initialFork = 0;
    int j = 0, i = 0;
    int tempPid = 0;
    int status;

    unsigned int *seconds = 0;
    unsigned int *nanoseconds = 0;
    unsigned int forkTimeSeconds = 0;
    unsigned int forkTimeNanoseconds = 0;
    unsigned int accessSpeed = 0;

    srand(time(NULL));
    char* filename = malloc(sizeof(char));
    filename = "logfile";
    FILE *infile = fopen(filename, "w");
    freopen("logfile","a",infile);

    int frameTable[256][4] = {{0}};

    key_t msgKey = ftok(".", 432820), timeKey = 0, rscKey = 0, semKey = 0;
    int msgid = msgget(msgKey, 0666 | IPC_CREAT), timeid = 0, rscID = 0, semid = 0, placementMarker = 0;

    memory_manager *rscPointer = NULL;
    sem_t *semPtr = NULL;
    makeShMemKey(&timeKey, &semKey, &rscKey);
    makeShMem(&timeid, &semid, &rscID, timeKey, semKey, rscKey);
    ShMemAttach(&seconds, &nanoseconds, &semPtr, &rscPointer, timeid, semid, rscID);
    double pageFaults = 0, memoryAccesses = 0, memoryAccessesPerSecond = 0;
    float childRequestAddress = 0;
    signal(SIGALRM, timekill);
    alarm(5);
    do {
        if(initialFork == 0){
            timeToFork(seconds, nanoseconds, &forkTimeSeconds, &forkTimeNanoseconds);
            initialFork = 1;
        }
        *nanoseconds += 50000;
        if(*nanoseconds >= 1000000000){
            *seconds += 1;
            *nanoseconds = 0;
            memoryAccessesPerSecond = (memoryAccesses/ *seconds);
        }
        if(((*seconds == forkTimeSeconds) && (*nanoseconds >= forkTimeNanoseconds)) || (*seconds > forkTimeSeconds)){
            if(arrayChecker(&placementMarker) == 1){
                forked++;
                initialFork = 0;
                rsg_manage_args(sharedTimeMem, sharedSemMem, sharedPositionMem, rscShrdMem, sharedLimitMem, sharedPercentageMem, timeid, semid, rscID, placementMarker, maxProcL, percentage);
                pid_t childPid = forkChild(sharedTimeMem, sharedSemMem, sharedPositionMem, rscShrdMem, sharedLimitMem, sharedPercentageMem);
                rscArraySz[placementMarker] = malloc(sizeof(struct memory_manager));
                (*rscArrayPointer)[placementMarker]->pid = childPid;

                for(i = 0 ; i < 32; i++){
                    (*rscArrayPointer)[placementMarker]->tableSz[i] = -1;
                }
                (*rscArrayPointer)[placementMarker]->resource_Marker = 1;

            }
        }
        for(i = 0; i < processCount; i++){

            if(setArr[i] == 1){

                tempPid =  (*rscArrayPointer)[i]->pid;

                if((msgrcv(msgid, &message, sizeof(message)-sizeof(long), tempPid, IPC_NOWAIT|MSG_NOERROR)) > 0){
                    if(atoi(message.msgChar) != 99999){
                        fprintf(infile, "OSS: P%d requesting read of address %d to ",i ,atoi(message.msgChar));
                        printf("OSS: P%d requesting read of address %d to ",i ,atoi(message.msgChar));
                        strcpy(childMsg, strtok(message.msgChar, " "));
                        address = atoi(childMsg);
                        strcpy(requestType, strtok(NULL, " "));
                        if(atoi(requestType) == 0){
                            fprintf(infile, "be read at time %d : %d\n", *seconds, *nanoseconds);
                            printf("be read at time %d : %d\n", *seconds, *nanoseconds);
                        }else{
                            fprintf(infile, "be written at time %d : %d\n", *seconds, *nanoseconds);
                            printf("be written at time %d : %d\n", *seconds, *nanoseconds);
                        }
                        childRequestAddress = (atoi(childMsg))/1000;
                        childRequestAddress = (int)(floor(childRequestAddress));
                        if((*rscArrayPointer)[i]->tableSz[(int)childRequestAddress] == -1 || frameTable[(*rscArrayPointer)[i]->tableSz[(int)childRequestAddress]][0] != (*rscArrayPointer)[i]->pid){//if the page table position is empty or the pagetable frame position no longer is associated with the child request address



                            frameLoop = 0;
                            while(frameTable[frameTablePos][0] != 0 && frameLoop < 255){
                                frameTablePos++;
                                frameLoop++;
                                if(frameTablePos == 256){
                                    frameTablePos = 0;
                                }
                                if(frameLoop == 255){
                                    pagefault = 1;
                                }
                            }
                            if(pagefault == 1){
                                pageFaults++;
                                fprintf(infile, "OSS: Address %d is not in a frame, page fault\n", address);
                                printf("OSS: Address %d is not in a frame, page fault\n", address);
                                while(frameTable[frameTablePos][1] != 0){
                                    frameTable[frameTablePos][1] = 0;
                                    frameTablePos++;
                                    if(frameTablePos == 256){
                                        frameTablePos = 0;
                                    }
                                }
                                if(frameTable[frameTablePos][1] == 0){
                                    memoryAccesses++;
                                    fprintf(infile, "OSS: Clearing frame %d and swapping in P%d page %d\n", frameTablePos, i, (int)childRequestAddress);
                                    printf("OSS: Clearing frame %d and swapping in P%d page %d\n", frameTablePos, i, (int)childRequestAddress);

                                    (*rscArrayPointer)[i]->tableSz[(int)childRequestAddress] = frameTablePos;
                                    frameTable[frameTablePos][0] = (*rscArrayPointer)[i]->pid;
                                    frameTable[frameTablePos][2] = atoi(requestType);
                                    fprintf(infile, "OSS: Address %d in frame %d giving data to P%d at time %d : %d\n", address, frameTablePos, i, *seconds, *nanoseconds);
                                    printf("OSS: Address %d in frame %d giving data to P%d at time %d : %d\n", address, frameTablePos, i, *seconds, *nanoseconds);
                                    frameTablePos++;
                                    if(frameTablePos == 256){
                                        frameTablePos = 0;
                                    }
                                    requests++;
                                }
                                accessSpeed +=  15000000;
                                *nanoseconds += 15000000;
                            } else {
                                memoryAccesses++;
                                (*rscArrayPointer)[i]->tableSz[(int)childRequestAddress] = frameTablePos;
                                frameTable[frameTablePos][0] = (*rscArrayPointer)[i]->pid;
                                frameTable[frameTablePos][1] = 0;
                                frameTable[frameTablePos][2] = atoi(requestType);
                                fprintf(infile, "OSS: Address %d in frame %d giving data to P%d at time %d : %d\n", address, frameTablePos, i, *seconds, *nanoseconds);
                                printf("OSS: Address %d in frame %d giving data to P%d at time %d : %d\n", address, frameTablePos, i, *seconds, *nanoseconds);
                                frameTablePos++;
                                if(frameTablePos == 256){
                                    frameTablePos = 0;
                                }
                                accessSpeed  += 10000000;
                                *nanoseconds += 10000000;
                                requests++;
                                fprintf(infile, "OSS: Dirty bit is set to %d and adding aditional time to the clock\n", atoi(requestType));
                                printf("OSS: Dirty bit is set to %d and adding aditional time to the clock\n", atoi(requestType));
                            }

                        } else {
                            memoryAccesses++;
                            frameTable[(*rscArrayPointer)[i]->tableSz[(int)childRequestAddress]][1] = 1; //Referenced bit set in function
                            frameTable[(*rscArrayPointer)[i]->tableSz[(int)childRequestAddress]][2] = atoi(requestType); //Dirty Bit is set
                            *nanoseconds += 10000000;
                            accessSpeed +=  10000000;
                            requests++;
                            fprintf(infile, "OSS: Dirty bit is set to %d  and adding aditional time to the clock\n", atoi(requestType));
                            printf("OSS: Dirty bit is set to %d  and adding aditional time to the clock\n", atoi(requestType));
                        }

                        message.msgString = ((*rscArrayPointer)[i]->pid+118);
                        sprintf(message.msgChar,"oss");
                        msgsnd(msgid, &message, sizeof(message)-sizeof(long), 0);

                    } else if(atoi(message.msgChar) == 99999){
                        setArr[i] = 0;
                        message.msgString = ((*rscArrayPointer)[i]->pid+118);
                        fprintf(infile, "OSS: P%d is complete! Clearing frames: ", i);
                        printf("OSS: P%d is complete! Clearing frames: ", i);
                        for(j = 0; j < 32; j++){

                            if((*rscArrayPointer)[i]->tableSz[j] != -1 && frameTable[(*rscArrayPointer)[i]->tableSz[j]][0] == (*rscArrayPointer)[i]->pid){
                                fprintf(infile, "%d, ", j);
                                printf("%d, ", j);
                                frameTable[(*rscArrayPointer)[i]->tableSz[j]][0] = 0;
                                frameTable[(*rscArrayPointer)[i]->tableSz[j]][1] = 0;
                                frameTable[(*rscArrayPointer)[i]->tableSz[j]][2] = 0;
                                (*rscArrayPointer)[i]->tableSz[j] = -1;
                            }
                        }
                        fprintf(infile,"\n");
                        printf("\n");
                        msgsnd(msgid, &message, sizeof(message)-sizeof(long), 0);
                        waitpid(((*rscArrayPointer)[i]->pid), &status, 0);
                        free(rscArraySz[i]);
                    }

                } else {
                }
            }
        }
        while((ch = fgetc(infile)) != EOF){
            if(ch == '\n'){
                lines++;
            }
        }
        if(lines >= 100000){
            fclose(infile);
        }

    }
    while((*seconds < SECOND_TIMER+10000) && alrm == 0 && forked < 100);
    fprintf(infile, "\nCurrent memory layout at time %d:%d is:\n", *seconds, *nanoseconds);
    fprintf(infile, "Occupied DirtyBit SecondChance NextFrame\n");
    printf("\nCurrent memory layout at time %d:%d is:\n", *seconds, *nanoseconds);
    printf("Occupied DirtyBit SecondChance NextFrame\n");
    for (int i = 0; i < 256; i++) {
        if (frameTable[i][0] != 0) {
            fprintf(infile, "Frame %d: %s %d %d %s\n", i, frameTable[i][1] == 1 ? "Yes" : "No", frameTable[i][2], frameTable[i][3], frameTable[i][3] == 0 ? "*" : "");
            printf("Frame %d: %s %d %d %s\n", i, frameTable[i][1] == 1 ? "Yes" : "No", frameTable[i][2], frameTable[i][3], frameTable[i][3] == 0 ? "*" : "");
        }
    }

    fclose(infile);
    shmdt(seconds);
    shmdt(semPtr);
    shmdt(rscPointer);
    msgctl(msgid, IPC_RMID, NULL);
    shmctl(msgid, IPC_RMID, NULL);
    shmctl(rscID, IPC_RMID, NULL);
    shmctl(timeid, IPC_RMID, NULL);
    shmctl(semid, IPC_RMID, NULL);
    kill(0, SIGTERM);
    return ( 0 );
}
