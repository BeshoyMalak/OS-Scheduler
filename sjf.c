
   if (!isEmpty(&SJF))
   {
   if( processTurn.remainingtime==0){
    SJF_file=fopen("SJF.txt","w");
    fprintf(SJF_file,"At Time %d Process %d exited arr %d total %d wait %d \n",currtime,processTurn.id, RunningProcess.arrivaltime,currtime-processTurn.arrivaltime);
    fclose(SJF_file);

   processTurn= peek(&SJF);

     SJF_file=fopen("SJF.txt","w");
    fprintf(SJF_file, "At time %d process %d started with running time %d ", currtime, processTurn.id, processTurn.runningtime);
    fclose(SJF_file);
   }


   kill(processTurn.id, SIGCONT);
   processTurn.remainingtime--;
   int shm_run = shmget(processTurn.id, 8, 0777);
    int *shmaddr_run = (int *) shmat(shm_run, (void *)0, 0);
    (*shmaddr_run)--;


   }
   
if (isEmpty(&SJF)){
   
if (processTurn.remainingtime==0 && processTurn.On==true){
   
    SJF_file=fopen("SJF.txt","w");
    fprintf(SJF_file,"At Time %d Process %d exited arr %d total %d wait %d \n",currtime,processTurn.id, RunningProcess.arrivaltime,currtime-processTurn.arrivaltime);
    fclose(SJF_file);
   
    printf("nothing to be scheduled for the moment");
   }
else if (processTurn.remainingtime>0 && processTurn.On==true){
  
     kill(processTurn.id, SIGCONT);
   processTurn.remainingtime--;
   int shm_run = shmget(processTurn.id, 8, 0777);
   int *shmaddr_run = (int *) shmat(shm_run, (void *)0, 0);
   (*shmaddr_run)--;
  }
}