#include "header.h"
#define timer 100

int alrm, total_proc = 18, frametable = 0;
int setArr[18] = {0};

struct memory_resource {
    long msgString;
    char msgChar[100];
} message;

void timekill(int sign_no){
    alrm = 1;
}

int array_lp(int *placementMarker){
    int inc = 0;
    for(inc = 0; inc < total_proc; inc++){
        if(setArr[inc] == 0){
            setArr[inc] = 1;
            *placementMarker = inc;
            return 1;
        }
    }
    return 0;
}

void time_fork(unsigned int *seconds, unsigned int *ns, unsigned int *forkTimeSeconds, unsigned int *forkTimeNanoseconds){
    unsigned int rnd = rand()%500000000;
    *forkTimeNanoseconds = 0;
    *forkTimeSeconds = 0;
    if((rnd + *ns) >= 1000000000){
        *forkTimeSeconds += 1;
        *forkTimeNanoseconds = (rnd + *ns) - 1000000000;
    } else {
        *forkTimeNanoseconds = rnd + *ns;
    }
    *forkTimeSeconds = *seconds;
}

int main (int argc, char *argv[]) {

    int percentage = 50;
    int max_proc = 100;

    char childMsg[20], ch;
    char requestType[20];
    char sharedPositionMem[10];
    char sharedPercentageMem[10];
    char sharedTimeMem[10];
    char ShrdMem[10];
    char sharedSemMem[10];
    char sharedLimitMem[10];

    int address;
    int forked = 0;
    int lines = 0;
    int frameLoop = 0;
    int pagefault = 0;
    int requests = 0;
    int ifork = 0;
    int j = 0, i = 0;
    int tempPid = 0;
    int status;

    unsigned int *seconds = 0;
    unsigned int *ns = 0;
    unsigned int forkTimeSeconds = 0;
    unsigned int forkTimeNanoseconds = 0;
    unsigned int access = 0;

    srand(time(NULL));
    char* filename = malloc(sizeof(char));
    filename = "logfile";
    FILE *fp = fopen(filename, "w");
    freopen("logfile","a",fp);

    int frameTable[256][4] = {{0}};

    key_t msgKey = ftok(".", 432820), timeKey = 0, Key = 0, semKey = 0;
    int msgid = msgget(msgKey, 0666 | IPC_CREAT), timeid = 0, ID = 0, semid = 0, placementMarker = 0;

    memory_manager *Pointer = NULL;
    sem_t *semPtr = NULL;
    makeShMemKey(&timeKey, &semKey, &Key);
    makeShMem(&timeid, &semid, &ID, timeKey, semKey, Key);
    ShMemAttach(&seconds, &ns, &semPtr, &Pointer, timeid, semid, ID);
    double pageFaults = 0, memoryAccesses = 0, memoryAccessesPerSecond = 0;
    float childRequestAddress = 0;
    signal(SIGALRM, timekill);
    alarm(5);
    do {
        if(ifork == 0){
            time_fork(seconds, ns, &forkTimeSeconds, &forkTimeNanoseconds);
            ifork = 1;
        }
        *ns += 50000;
        if(*ns >= 1000000000){
            *seconds += 1;
            *ns = 0;
            memoryAccessesPerSecond = (memoryAccesses/ *seconds);
        }
        if(((*seconds == forkTimeSeconds) && (*ns >= forkTimeNanoseconds)) || (*seconds > forkTimeSeconds)){
            if(array_lp(&placementMarker) == 1){
                forked++;
                ifork = 0;
                rsg_manage_args(sharedTimeMem, sharedSemMem, sharedPositionMem, ShrdMem, sharedLimitMem, sharedPercentageMem, timeid, semid, ID, placementMarker, max_proc, percentage);
                pid_t childPid = forkChild(sharedTimeMem, sharedSemMem, sharedPositionMem, ShrdMem, sharedLimitMem, sharedPercentageMem);
                array_size[placementMarker] = malloc(sizeof(struct memory_manager));
                (*ArrayPointer)[placementMarker]->pid = childPid;

                for(i = 0 ; i < 32; i++){
                    (*ArrayPointer)[placementMarker]->tablesize[i] = -1;
                }
                (*ArrayPointer)[placementMarker]->resource_Marker = 1;

            }
        }
        for(i = 0; i < total_proc; i++){

            if(setArr[i] == 1){

                tempPid =  (*ArrayPointer)[i]->pid;

                if((msgrcv(msgid, &message, sizeof(message)-sizeof(long), tempPid, IPC_NOWAIT|MSG_NOERROR)) > 0){
                    if(atoi(message.msgChar) != 99999){
                        fprintf(fp, "OSS: P%d requesting read of address %d to ",i ,atoi(message.msgChar));
                        printf("OSS: P%d requesting read of address %d to ",i ,atoi(message.msgChar));
                        strcpy(childMsg, strtok(message.msgChar, " "));
                        address = atoi(childMsg);
                        strcpy(requestType, strtok(NULL, " "));
                        if(atoi(requestType) == 0){
                            fprintf(fp, "be read at time %d : %d\n", *seconds, *ns);
                            printf("be read at time %d : %d\n", *seconds, *ns);
                        }else{
                            fprintf(fp, "be written at time %d : %d\n", *seconds, *ns);
                            printf("be written at time %d : %d\n", *seconds, *ns);
                        }
                        childRequestAddress = (atoi(childMsg))/1000;
                        childRequestAddress = (int)(floor(childRequestAddress));
                        if((*ArrayPointer)[i]->tablesize[(int)childRequestAddress] == -1 || frameTable[(*ArrayPointer)[i]->tablesize[(int)childRequestAddress]][0] != (*ArrayPointer)[i]->pid){//if the page table position is empty or the pagetable frame position no longer is associated with the child request address



                            frameLoop = 0;
                            while(frameTable[frametable][0] != 0 && frameLoop < 255){
                                frametable++;
                                frameLoop++;
                                if(frametable == 256){
                                    frametable = 0;
                                }
                                if(frameLoop == 255){
                                    pagefault = 1;
                                }
                            }
                            if(pagefault == 1){
                                pageFaults++;
                                fprintf(fp, "OSS: Address %d is not in a frame, page fault\n", address);
                                printf("OSS: Address %d is not in a frame, page fault\n", address);
                                while(frameTable[frametable][1] != 0){
                                    frameTable[frametable][1] = 0;
                                    frametable++;
                                    if(frametable == 256){
                                        frametable = 0;
                                    }
                                }
                                if(frameTable[frametable][1] == 0){
                                    memoryAccesses++;
                                    fprintf(fp, "OSS: Clearing frame %d and swapping in P%d page %d\n", frametable, i, (int)childRequestAddress);
                                    printf("OSS: Clearing frame %d and swapping in P%d page %d\n", frametable, i, (int)childRequestAddress);

                                    (*ArrayPointer)[i]->tablesize[(int)childRequestAddress] = frametable;
                                    frameTable[frametable][0] = (*ArrayPointer)[i]->pid;
                                    frameTable[frametable][2] = atoi(requestType);
                                    fprintf(fp, "OSS: Address %d in frame %d giving data to P%d at time %d : %d\n", address, frametable, i, *seconds, *ns);
                                    printf("OSS: Address %d in frame %d giving data to P%d at time %d : %d\n", address, frametable, i, *seconds, *ns);
                                    frametable++;
                                    if(frametable == 256){
                                        frametable = 0;
                                    }
                                    requests++;
                                }
                                access +=  15000000;
                                *ns += 15000000;
                            } else {
                                memoryAccesses++;
                                (*ArrayPointer)[i]->tablesize[(int)childRequestAddress] = frametable;
                                frameTable[frametable][0] = (*ArrayPointer)[i]->pid;
                                frameTable[frametable][1] = 0;
                                frameTable[frametable][2] = atoi(requestType);
                                fprintf(fp, "OSS: Address %d in frame %d giving data to P%d at time %d : %d\n", address, frametable, i, *seconds, *ns);
                                printf("OSS: Address %d in frame %d giving data to P%d at time %d : %d\n", address, frametable, i, *seconds, *ns);
                                frametable++;
                                if(frametable == 256){
                                    frametable = 0;
                                }
                                access  += 10000000;
                                *ns += 10000000;
                                requests++;
                                fprintf(fp, "OSS: Dirty bit is set to %d and adding aditional time to the clock\n", atoi(requestType));
                                printf("OSS: Dirty bit is set to %d and adding aditional time to the clock\n", atoi(requestType));
                            }

                        } else {
                            memoryAccesses++;
                            frameTable[(*ArrayPointer)[i]->tablesize[(int)childRequestAddress]][1] = 1;
                            frameTable[(*ArrayPointer)[i]->tablesize[(int)childRequestAddress]][2] = atoi(requestType); 
                            *ns += 10000000;
                            access +=  10000000;
                            requests++;
                            fprintf(fp, "OSS: Dirty bit is set to %d  and adding aditional time to the clock\n", atoi(requestType));
                            printf("OSS: Dirty bit is set to %d  and adding aditional time to the clock\n", atoi(requestType));
                        }

                        message.msgString = ((*ArrayPointer)[i]->pid+118);
                        sprintf(message.msgChar,"oss");
                        msgsnd(msgid, &message, sizeof(message)-sizeof(long), 0);

                    } else if(atoi(message.msgChar) == 99999){
                        setArr[i] = 0;
                        message.msgString = ((*ArrayPointer)[i]->pid+118);
                        fprintf(fp, "OSS: P%d is complete! Clearing frames: ", i);
                        printf("OSS: P%d is complete! Clearing frames: ", i);
                        for(j = 0; j < 32; j++){

                            if((*ArrayPointer)[i]->tablesize[j] != -1 && frameTable[(*ArrayPointer)[i]->tablesize[j]][0] == (*ArrayPointer)[i]->pid){
                                fprintf(fp, "%d, ", j);
                                printf("%d, ", j);
                                frameTable[(*ArrayPointer)[i]->tablesize[j]][0] = 0;
                                frameTable[(*ArrayPointer)[i]->tablesize[j]][1] = 0;
                                frameTable[(*ArrayPointer)[i]->tablesize[j]][2] = 0;
                                (*ArrayPointer)[i]->tablesize[j] = -1;
                            }
                        }
                        fprintf(fp,"\n");
                        printf("\n");
                        msgsnd(msgid, &message, sizeof(message)-sizeof(long), 0);
                        waitpid(((*ArrayPointer)[i]->pid), &status, 0);
                        free(array_size[i]);
                    }

                } else {
                }
            }
        }
        while((ch = fgetc(fp)) != EOF){
            if(ch == '\n'){
                lines++;
            }
        }
        if(lines >= 100000){
            fclose(fp);
        }

    }
    while((*seconds < timer+10000) && alrm == 0 && forked < 100);
    fprintf(fp, "\nCurrent memory layout at time %d:%d is:\n", *seconds, *ns);
    fprintf(fp, "Occupied DirtyBit SecondChance NextFrame\n");
    printf("\nCurrent memory layout at time %d:%d is:\n", *seconds, *ns);
    printf("Occupied DirtyBit SecondChance NextFrame\n");
    for (int i = 0; i < 256; i++) {
        if (frameTable[i][0] != 0) {
            fprintf(fp, "Frame %d: %s %d %d %s\n", i, frameTable[i][1] == 1 ? "Yes" : "No", frameTable[i][2], frameTable[i][3], frameTable[i][3] == 0 ? "*" : "");
            printf("Frame %d: %s %d %d %s\n", i, frameTable[i][1] == 1 ? "Yes" : "No", frameTable[i][2], frameTable[i][3], frameTable[i][3] == 0 ? "*" : "");
        }
    }

    fclose(fp);
    shmdt(seconds);
    shmdt(semPtr);
    shmdt(Pointer);
    msgctl(msgid, IPC_RMID, NULL);
    shmctl(msgid, IPC_RMID, NULL);
    shmctl(ID, IPC_RMID, NULL);
    shmctl(timeid, IPC_RMID, NULL);
    shmctl(semid, IPC_RMID, NULL);
    kill(0, SIGTERM);
    return ( 0 );
}
