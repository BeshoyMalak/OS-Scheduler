#include "Headers/headers.h"
#include "Headers/ProcessStruct.h"
#include "Headers/ProcessHeap.h"
#include "Headers/MessageBuffer.h"
#include "Headers/EventsQueue.h"
#include "Headers/ProcessQueue.h"
#include <math.h>

event_queue gEventQueue;
heap_t *gProcessHeap;
queue gProcessQueue;
Process *gpCurrentProcess = NULL;

Process *pProcess = NULL; //store coming process in
int gFirstTimeAddFlag=0;
int switch_context = 0; //this flag is to excute the following process
//Process *gTestP = NULL;

void ProcessArrivalHandler(int);

void InitIPC();

int ReceiveProcess();

void ExecuteProcess();

void CleanResources();

void AddEvent(enum EventType);

void ChildHandler(int signum);

int gMsgQueueId = 0;

void LogEvents(unsigned int start_time, unsigned int end_time);


int main(int argc, char *argv[]) {
    printf("HPF: *** Scheduler here\n");
    initClk(); ///printf("*** Clk Initiated in Sch successfully\n");
    InitIPC();
    //initialize processes heap
    gProcessHeap = (heap_t *) calloc(1, sizeof(heap_t));
    //initialize EventQueu
    gEventQueue= NewEventQueue();
    signal(SIGUSR1, ProcessArrivalHandler);
    signal(SIGINT, CleanResources);
    signal(SIGCHLD, ChildHandler);
    gpCurrentProcess->mPriority = 15;

    gFirstTimeAddFlag=0;
    int mTempRunTime=0;
    char mTempSRT[20];
    int StartSimulationTime = getClk();
    while (!HeapEmpty(gProcessHeap)) {
        printf("HPF in the while loop");
        gpCurrentProcess= HeapPop(gProcessHeap);

        ExecuteProcess();
        switch_context = 0; //turn off context switching
                while (!switch_context) //as long as this flag is set to zero keep pausing
                    pause(); //to avoid busy waiting only wakeup to serve signals
            }
        switch_context = 0; //turn off context switching
        ExecuteProcess(); //execute current process
        while (!switch_context){ //as long as this flag is set to zero keep pausing
            pause(); //to avoid busy waiting only wakeup to serve signals
        }
    int EndtSimulationTime = getClk();

    LogEvents(StartSimulationTime, EndtSimulationTime);
    printf("HPF: done");
    raise(SIGINT); //when it's done raise SIGINT to clean and exit
    //printf("The clock after the run = %d \n", getClk());
}

void ProcessArrivalHandler(int signum) {
    while(!ReceiveProcess());
}

void InitIPC() {
    key_t key = ftok(gFtokFile, gFtokCode);
    gMsgQueueId = msgget(key, 0);
    if (gMsgQueueId == -1) {
        printf("HPF: *** Scheduler IPC init failed");
        raise(SIGINT);
    }
    printf("HPF: *** Scheduler IPC ready!\n");
}

int ReceiveProcess() {
    Message msg;
    if (msgrcv(gMsgQueueId, (void *)&msg, sizeof(msg.mProcess),0, IPC_NOWAIT) == -1){
        perror("HPF: *** Error in receive");
        return -1;
    }
    //msg = msgrcv(gMsgQueueId, (void *)&msg, sizeof(msg.mProcess),0, IPC_NOWAIT);
    printf(" till here, msg is received well in scheduler \n");
    Process *pProcess = malloc(sizeof(Process));
    while(!pProcess){
        printf("There is a problem in allocating memory for the new messages, retrying Now\n");
        pProcess = malloc(sizeof(Process));
    }

    *pProcess= msg.mProcess;
    if(!pProcess){
        printf("Error in allocation of the message\n");
    }
    if(gpCurrentProcess->mPriority > msg.mProcess.mPriority){    //if the coming process has priority more than the current one
        if (!kill(gpCurrentProcess->mPid, SIGTSTP) == -1){ //stop current process
            printf("HPF: current proc. is stoped due to another proc. priority");
            HeapPush(gProcessHeap, gpCurrentProcess->mPriority, gpCurrentProcess);
            AddEvent(STOP);
            switch_context = 1;
            return 0;
        }
        else{ printf("error in stopping the process"); return 0;}
    }
    HeapPush(gProcessHeap, pProcess->mPriority, pProcess);
  //  printf("Process is now pushed in the Heap \n");
    return 0;

}

void CleanResources(int signum) {
    printf("HPF: *** Cleaning scheduler resources\n");

    while (HeapPeek(gProcessHeap)) //while processes heap is not empty
        free(pProcess); //free memory allocated by this process

    Event *pEvent = NULL;
    while (EventQueueDequeue(gEventQueue, &pEvent)) //while event queue is not empty
        free(pEvent); //free memory allocated by the event
    printf("HPF: *** Scheduler clean!\n");
    exit(EXIT_SUCCESS);
};


void AddEvent(enum EventType type) {
    Event *pEvent = malloc(sizeof(Event));
    while (!pEvent) {
        perror("HPF: *** Malloc failed");
        printf("HPF: *** Trying again");
        pEvent = malloc(sizeof(Event));
    }
    pEvent->mTimeStep = getClk();
    printf("Event has been created and timestep assigned \n");
    if (type == FINISH) {
        pEvent->mTaTime = getClk() - gpCurrentProcess->mArrivalTime;
        pEvent->mWTaTime = (double) pEvent->mTaTime / gpCurrentProcess->mRuntime;
    }
    pEvent->mpProcess = gpCurrentProcess;
    pEvent->mType = type;
    //printf("Current event type is ...\n");
    pEvent->mCurrentRemTime = gpCurrentProcess->mRemainTime;
    pEvent->mCurrentWaitTime = gpCurrentProcess->mWaitTime;
    //printf("Event is about to be created");
    EventQueueEnqueue(gEventQueue, pEvent);
    //printf("Event inserted successfully in the Queue \n");
    printf("EVENT HAS BEEN ADDED TO QUEUE \n");

    //PrintEvent(pEvent);
}

void ExecuteProcess() {
    if (gpCurrentProcess->mRuntime == gpCurrentProcess->mRemainTime) { //if it's the first time to excute this process
    gpCurrentProcess->mPid = fork(); //fork a new child and store its pid in the process struct
    while (gpCurrentProcess->mPid == -1) { //if forking fails
        perror("HPF: *** Error forking process");
        printf("HPF: *** Trying again...\n");
        gpCurrentProcess->mPid = fork();
    }
    if (!gpCurrentProcess->mPid) {
        printf("&&& START TIME OF THE CURRENT PROCESS (BEGIN OF CHILD): %d \n", getClk());
        char buffer[10]; //buffer to convert runtime from int to string
        sprintf(buffer, "%d", gpCurrentProcess->mRuntime);
        char *argv[] = {"process.out", buffer, NULL};
        execv("process.out", argv);
        perror("HPF: *** Process execution failed");

        // printf("REACHED the END of the CHILD at excute, AKA: a PROCESS RUN \n");
        //printf("THE CLK NOW AT CHILD END = %d\n", getClk());
        exit(EXIT_FAILURE);
    }
    gpCurrentProcess->mWaitTime = getClk() - gpCurrentProcess->mArrivalTime;
    AddEvent(START); //create an event
    }
    else{ //if it is stopped before and now needs to be resumed
        if (kill(gpCurrentProcess->mPid, SIGCONT) == -1) { //continue process
            printf("HPF: *** Error resuming process %d", gpCurrentProcess->mId);
            perror(NULL);
            return;
        }
        gpCurrentProcess->mWaitTime += getClk() - gpCurrentProcess->mLastStop; //add the additional waiting time
        AddEvent(CONT);
    }

}

void LogEvents(unsigned int start_time, unsigned int end_time) {
    unsigned int runtime_sum = 0, waiting_sum = 0, count = 0;
    double wta_sum = 0, wta_squared_sum = 0;
    FILE *pFile = fopen("Events.txt", "w");
    Event *pEvent = NULL;
    while (EventQueueDequeue(gEventQueue, &pEvent)) { //while event queue is not empty
        PrintEvent(pEvent);
        OutputEvent(pEvent, pFile);
        if (pEvent->mType == FINISH) {
            runtime_sum += pEvent->mpProcess->mRuntime;
            waiting_sum += pEvent->mCurrentWaitTime;
            count++;
            wta_sum += pEvent->mWTaTime;
            wta_squared_sum += pEvent->mWTaTime * pEvent->mWTaTime;
            free(pEvent->mpProcess);
        }
        free(pEvent); //free memory allocated by the event
    }
    fclose(pFile);
    double cpu_utilization = runtime_sum * 100.0 / (end_time - start_time);
    double avg_wta = wta_sum / count;
    double avg_waiting = (double) waiting_sum / count;
    double std_wta = sqrt((wta_squared_sum - (2 * wta_sum * avg_wta) + (avg_wta * avg_wta * count)) / count);

    printf("\nCPU utilization = %.2f\n", cpu_utilization);
    printf("Avg WTA = %.2f\n", avg_wta);
    printf("Avg Waiting = %.2f\n", avg_waiting);
    printf("STD WTA = %.2f\n\n", std_wta);
}

void ChildHandler(int signum) {
    if (!waitpid(gpCurrentProcess->mPid, NULL, WNOHANG)) //if current process did not terminate
       { return;}

    gpCurrentProcess->mRemainTime = 0; //process finished so remaining time should be zero
    AddEvent(FINISH);
    switch_context = 1; //set flag to 1 so main loop knows it's time to switch context
}