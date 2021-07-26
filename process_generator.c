#include "Headers/headers.h"
#include "Headers/ProcessQueue.h"
#include "Headers/MessageBuffer.h"
#include <string.h>
#include "math.h"

void ClearResources(int);

void ReadFile();

void InitIPC();

int GetUserChoice();

void ExecuteClock();

void ExecuteScheduler(int, int);

void SendProcess(Process *);

queue gProcessQueue;
int gMsgQueueId = 0;
pid_t gClockPid = 0;
pid_t gSchedulerPid = 0;

int main(int argc, char *argv[]) {
    //initialize the process queue
    gProcessQueue = NewProcQueue();
    //catch SIGINT
    signal(SIGINT, ClearResources);
    // 1. Read the input files.
    ReadFile();
    // 2. Ask the user for the chosen scheduling algorithm and its parameters, if there are any.
    int choice = GetUserChoice(); //1 = HPF, 2 = SRTN, 3 = RR
    while (choice < 1 || choice > 3) {
        printf("PG: *** Invalid choice\n");
        choice = GetUserChoice();
    }
    int quanta = 0;
    if (choice == 3) {
        printf("Enter quanta:\n");
        scanf("%d", &quanta);
    }
	
	
    //initialize the IPC
    InitIPC();
    // 3. Initiate and create the scheduler and clock processes.
	
    ExecuteScheduler(choice, quanta);
    ExecuteClock();
    // 4. Use this function after creating the clock process to initialize clock
    initClk();
    // To get time use this
	
	
    while (!ProcQueueEmpty(gProcessQueue)) {
        //get current time
        int current_time = getClk();
        //temporary process pointer
        Process *pTempProcess;
        //peek the processes queue
        ProcPeek(gProcessQueue, &pTempProcess);
        //keep looping as long as the process on top has arrival time equal to the current time
        bool is_time = false; //flag to indicate whether at least one process matches current time or not
		//the case that 2 or more processes come at the same time is handled
        while (pTempProcess->mArrivalTime == current_time) {
            is_time = true;
            SendProcess(pTempProcess); //send this process to the scheduler
            ProcDequeue(gProcessQueue, &pTempProcess); //dequeue this process from the processes queue
            free(pTempProcess); //free memory allocated by this process
            ProcPeek(gProcessQueue, &pTempProcess); //peek the next process in the queue
        }
        if (is_time) //if at least one process was sent to the scheduler
            kill(gSchedulerPid, SIGUSR1); //send SIGUSR1 to the scheduler
        usleep(900000); //sleep 0.9 sec 1000,000 <==> 1 sec
    }
    // invoke ClearResources() but use zero as parameter to indicate normal exit not interrupt
    ClearResources(0);
}

int GetUserChoice() {
    int decision;
    printf("\nPG: *** Choose scheduling algorithm:\n");
    printf("1. Highest Priority First\n2. Shortest Remaining Time Next\n3. Round Robin\n");
    scanf("%d", &decision);
    return decision;
}

void ClearResources(int signum) {
    //Clear IPC resources
    if (gMsgQueueId != 0) {
        printf("*** Cleaning IPC resources...\n");
        if (msgctl(gMsgQueueId, IPC_RMID, NULL) == -1)
            perror("*** Error");
        else
            printf("*** IPC cleaned!\n");
    }

    //free queue memory
    printf("*** Cleaning processes queue...\n");
    Process *pTemp;
    while (ProcDequeue(gProcessQueue, &pTemp)) {
        free(pTemp);
    }
    printf("*** Process queue cleaned!\n");

    //if this function is invoked due to an interrupt signal then immediately interrupt all processes
    if (signum == SIGINT) {
        //Interrupt forked processes
        printf("*** Sending interrupt to scheduler\n");
        if (gSchedulerPid)
            kill(gSchedulerPid, SIGINT);
        printf("*** Sending interrupt to clock\n");
        if (gClockPid) {
            destroyClk(false);
            kill(gClockPid, SIGINT);
        }
        //do not leave before both forked processes are done
        wait(NULL);
        wait(NULL);
    } else { //we need to wait until Scheduler exits by itself
        printf("*** Waiting for scheduler to do its job...\n");
        waitpid(gSchedulerPid, NULL, 0); //wait until scheduler exits
        printf("*** Scheduler exit signal received\n");
        printf("*** Sending interrupt to clock\n");
        if (gClockPid) {
            destroyClk(false);
            kill(gClockPid, SIGINT);
        }
        //do not leave before clock is done
        wait(NULL);
    }
    printf("*** Clean!\n");
    exit(EXIT_SUCCESS);
}

void ReadFile() {
    printf("PG: *** Attempting to open input file...\n");
    FILE *pFile;
    char *pLine = NULL;
    size_t len = 0;
    /**/size_t read;
    pFile = fopen("processes.txt", "r");
    if (pFile == NULL) {
        perror("*** Error reading from input file");
        exit(EXIT_FAILURE);
    }
    printf("*** Reading input file...\n");
    unsigned int runtime_sum = 0, runtime_squared_sum = 0, count = 0;
    while ((read = getline(&pLine, &len, pFile)) != -1) {
        if (pLine[0] == '#') //skip hashes as they are just comments in the input file
            continue;

        Process *pProcess = malloc(sizeof(Process));
        while (!pProcess) {
            perror("*** Malloc failed");
            printf("*** Trying again");
            pProcess = malloc(sizeof(Process));
        }
		
//fill the processes data => convert string of char to int in order to use it
        pProcess->mId = atoi(strtok(pLine, "\t"));
        pProcess->mArrivalTime = atoi(strtok(NULL, "\t"));
        pProcess->mRuntime = atoi(strtok(NULL, "\t"));
        pProcess->mPriority = atoi(strtok(NULL, "\t"));
        pProcess->mRemainTime = pProcess->mRuntime;
        pProcess->mWaitTime = 0;
        runtime_sum += pProcess->mRuntime;
        runtime_squared_sum += pProcess->mRuntime * pProcess->mRuntime;
        count++;
        ProcEnqueue(gProcessQueue, pProcess);
    }

    double runtime_avg = (double) runtime_sum / count;
    double runtime_std = sqrt(
            (runtime_squared_sum - (2 * runtime_sum * runtime_avg) + (runtime_avg * runtime_avg * count)) / count);
    printf("*** Releasing file resources...\n");
    fclose(pFile);
    if (pLine)
        free(pLine);
    printf("*** Input file done successfully!\n");
    printf("\nPG: *** Total runtime %d/s, %.2f/m, %.2f/h\n", runtime_sum, runtime_sum / 60.0, runtime_sum / (60.0 * 60.0));
    printf("*** Average runtime = %.2f, STD = %.2f\n", runtime_avg, runtime_std);
}

void InitIPC() {
    key_t key = ftok(gFtokFile, gFtokCode);
    gMsgQueueId = msgget(key, IPC_CREAT | 0666);
    if (gMsgQueueId == -1) {
        perror("*** IPC init failed");
        raise(SIGINT);
    }
    printf("*** IPC ready!\n");
}

void ExecuteClock() {
    gClockPid = fork();
    while (gClockPid == -1) {
        perror("*** Error forking clock");
        printf("*** Trying again...\n");
        gClockPid = fork();
    }
    if (gClockPid == 0) {
        printf("*** Clock forking done!\n");
        printf("*** Executing clock...\n");
        char *argv[] = {"clk.out", NULL};
        execv("clk.out", argv);
        perror("*** Clock execution failed");
        exit(EXIT_FAILURE);
    }

}

void ExecuteScheduler(int type, int quanta) {
    gSchedulerPid = fork();
    while (gSchedulerPid == -1) {
        perror("*** Error forking scheduler");
        printf("*** Trying again...\n");
        gSchedulerPid = fork();
    }
    if (gSchedulerPid == 0) {
        printf("*** Scheduler forking done!\n");
        printf("*** Executing scheduler...\n");
        char *argv[3];
        argv[1] = NULL;
        char buffer[10];
        switch (type) {
            case 1:
                argv[0] = "hpf.out";
                execv("hpf.out", argv);
                break;
            case 2:
                argv[0] = "srtn.out";
                execv("srtn.out", argv);
                break;
            case 3:
                sprintf(buffer, "%d", quanta);
                argv[2] = NULL;
                argv[1] = buffer;
                argv[0] = "roundrobin.out";
                execv("roundrobin.out", argv);
                break;
            default:
                break;

			
        }
        perror("*** Scheduler execution failed");
        exit(EXIT_FAILURE);
    }
}

void SendProcess(Process *pProcess) {
    Message msg;
    msg.mType = 10; //arbitrary value for mType
    msg.mProcess = *pProcess;
    printf("*** Sending process with id %d to scheduler...\n", msg.mProcess.mId);
    if (msgsnd(gMsgQueueId, (void *) &msg, sizeof(msg.mProcess), !IPC_NOWAIT) == -1) {
        perror("*** Error while sending process");
    } else {
        printf("*** Process sent!\n");
    }
}