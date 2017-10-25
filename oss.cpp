#include "oss.h"

int main (int argc, char **argv)
{
  srand(time(0));
  mArg = (char*) malloc(20);
  nArg = (char*) malloc(20);
  pArg = (char*) malloc(20);
  rArg = (char*) malloc(20);
  tArg = (char*) malloc(20);
  int hflag = 0;
  int nonOptArgFlag = 0;
  int index;
  char *filename = "logfile.txt";
  char *defaultFileName = "logfile.txt";
  char *programName = argv[0];
  char *option = NULL;
  int c;

  
  //process arguments
  while ((c = getopt(argc, argv, ":hs:l:t:")) != -1)
    switch (c) {
      case 'h':
      printHelpMessage();
      return(1);
        break;
      case 's':
        sValue = atoi(optarg);
        if(sValue > MAXuser) {
          sValue = 20;
          fprintf(stderr, "No more than 20 user processes allowed at a time. Reverting to 20.\n");
        }
        break;
      case 'l':
        filename = optarg;
        break;
      case 't':
        tValue = atoi(optarg);
        break;
      default:
              printHelpMessage();
              return(-1);
    }    

  //Initialize the alarm and CTRL-C handler
  signal(SIGALRM, interruptHandler);
  signal(SIGINT, interruptHandler);
  signal(SIGCHLD, SIG_IGN);
  signal(SIGQUIT, SIG_IGN);

  //set the alarm to tValue seconds
  alarm(tValue);

  int sizeArray = sizeof(*pcbArray) * 18;
  int sizeResource = sizeof(*resourceArray) * 20;

  //Try to get the shared mem id from the key with a size of the struct
  //create it with all perms
  if((shmid = shmget(timerKey, sizeof(sharedStruct), IPC_CREAT | 0777)) == -1) {
    perror("Bad shmget allocation shared struct");
    exit(-1);
  }

  //Try to attach the struct pointer to shared memory
  if((myStruct = (struct sharedStruct *)shmat(shmid, NULL, 0)) == (void *) -1) {
    perror("oss could not attach shared mem");
    exit(-1);
  }
  
  //get shmid for pcbArray of 18 pcbs
  if((pcbShmid = shmget(pcbArrayKey, sizeArray, IPC_CREAT | 0777)) == -1) {
    perror("Bad shmget allocation pcb array");
    exit(-1);
  }

  //try to attach pcb array to shared memory
  if((pcbArray = (struct PCB *)shmat(pcbShmid, NULL, 0)) == (void *) -1) {
    perror("oss could not attach to pcb array");
    exit(-1);
  }

  if((resourceShmid = shmget(resourceKey, sizeResource, IPC_CREAT | 0777)) == -1) {
    perror("Bad shmget allocation resource array");
    exit(-1);
  }

  if((resourceArray = (struct resource *)shmat(resourceShmid, NULL, 0)) == (void *) -1) {
    perror("oss could not attach to resource array");
    exit(-1);
  }

  //create message queue for the oss process
  if((ossQueueId = msgget(ossQueueKey, IPC_CREAT | 0777)) == -1) {
    perror("oss msgget for oss queue");
    exit(-1);
  }


  int i;
  for (i = 0; i < ARRAY_SIZE; i++) {
    pcbArray[i].processID = 0;
    pcbArray[i].request = -1;
    pcbArray[i].release = -1;
    pcbArray[i].totalTimeRan = 0;
    pcbArray[i].createTime = 0;
    int j;
    for(j = 0; j < 20; j++) {
      pcbArray[i].allocation.type[j] = -1;
      pcbArray[i].allocation.quantity[j] = 0;
    }
  }

  //Open file and mark the beginning of the new log
  file = fopen(filename, "w");
  if(!file) {
    perror("Error opening file");
    exit(-1);
  }

  myStruct->ossTimer = 0;
  myStruct->sigNotReceived = 1;

  int tempIncrement;
  do {

    if(isTimeToSpawn()) {
      spawnuser();
      setTimeToSpawn();
    }

    myStruct->ossTimer += incrementTimer();

    updateProcess(processSystem());
  
  } while (myStruct->ossTimer < MAX_TIME && myStruct->sigNotReceived);

  if(!cleanupCalled) {
    cleanupCalled = 1;
    cleanup();
  }
  return 0;
}

/**
 * Author: Jason Klamert
 * Date: 11/26/2016
 * Description: Function to check if it is time to spawn users.
 **/
bool isTimeToSpawn(void) {
  printf("Checking time to spawn: future = %llu.%09llu timer = %llu.%09llu\n" , timeToSpawn / NANO_MODIFIER, timeToSpawn % NANO_MODIFIER, myStruct->ossTimer / NANO_MODIFIER, myStruct->ossTimer % NANO_MODIFIER);
  fprintf(file, "Checking time to spawn: future = %llu timer = %llu\n", timeToSpawn, myStruct->ossTimer);
  
  if(myStruct->ossTimer >= timeToSpawn)
  return true;
  else
  return false;
}

/**
 * Author: Jason Klamert
 * Date: 11/26/2016
 * Description: Function to set time to spawn users.
 **/
void setTimeToSpawn(void) {
  timeToSpawn = myStruct->ossTimer + rand() % MAX_FUTURE_SPAWN;
  printf("Will try to spawn user at time %llu.%09llu\n", timeToSpawn / NANO_MODIFIER, timeToSpawn % NANO_MODIFIER);
  fprintf(file, "Will try to spawn user at time %llu\n", timeToSpawn);
}

/**
 * Author: Jason Klamert
 * Date: 11/26/2016
 * Description: Function to spawn users.
 **/
void spawnuser(void) {

    processNumberBeingSpawned = -1;
    
    int i;
    for(i = 0; i < ARRAY_SIZE; i++) {
      if(pcbArray[i].processID == 0) {
        processNumberBeingSpawned = i;
        pcbArray[i].processID = 1;
        break;
      } 
    }

    if(processNumberBeingSpawned == -1) {
      printf("PCB array is full. No process created.\n");
      fprintf(file, "PCB array is full. No process created.\n");
    }

    if(processNumberBeingSpawned != -1) {
      printf("Found open PCB. Spawning process.\n");
      totalProcessesSpawned = totalProcessesSpawned + 1;

      if((childPid = fork()) < 0) {
        perror("Fork Failure");
      }

      if(childPid == 0) {
        printf("Total processes spawned: %d\n", totalProcessesSpawned);
        pcbArray[processNumberBeingSpawned].createTime = myStruct->ossTimer;
        pcbArray[processNumberBeingSpawned].processID = getpid();
        sprintf(mArg, "%d", shmid);
        sprintf(nArg, "%d", processNumberBeingSpawned);
        sprintf(pArg, "%d", pcbShmid);
        sprintf(rArg, "%d", resourceShmid);
        sprintf(tArg, "%d", tValue);
        char *userOptions[] = {"./userrunner", "-m", mArg, "-n", nArg, "-p", pArg, "-r", rArg, "-t", tArg, (char *)0};
        execv("./userrunner", userOptions);
      }
      
    }
}

/**
 * Author: Jason Klamert
 * Date: 11/26/2016
 * Description: Incrememts oss timer.
 **/
int incrementTimer(void) {
  int random = 1 + rand() % MAX_IDLE_INCREMENT;
  return random;
}

/**
 * Author: Jason Klamert
 * Date: 11/26/2016
 * Description: Get messages for oss.
 **/
int processSystem(void) {
  struct msgformat msg;

  if(msgrcv(ossQueueId, (void *) &msg, sizeof(msg.mText), 3, IPC_NOWAIT) == -1) {
    if(errno != ENOMSG) {
      perror("Error oss receiving message");
      return -1;
    }
    printf("No message for oss\n");
    return -1;
  }
  else {
    int processNum = atoi(msg.mText);
    return processNum;
  }
}

/**
 * Author: Jason Klamert
 * Date: 11/26/2016
 * Description: update the process turn around time.
 **/
void updateAverageTT(int pcb) {

  long long startToFinish = myStruct->ossTimer - pcbArray[pcb].createTime;
  totalProcessLifeTime += startToFinish;
}

/**
 * Author: Jason Klamert
 * Date: 11/26/2016
 * Description: update the process in the PCB.
 **/
void updateProcess(int processLocation) {

  //If no message received, no process performed any actions this check. Return.
  if(processLocation == -1) {
    return;
  }

  pid_t id = pcbArray[processLocation].processID;

  //If you made it here and the processID is not 0, check for either a 
  //request or release
  if(id != 0) {
    int request = pcbArray[processLocation].request;
    int release = pcbArray[processLocation].release;
    //If request flag is set, try to give that process the requested resource
    if(request > -1) {
      //If there are available resources, allocate them
      if(resourceArray[request].quantAvail > 0) {
        pcbArray[processLocation].request = -1;
        resourceArray[request].quantAvail -= 1;
        int i;
        int openSpace = -1;
        int foundExisting = 0;
        //See if the process already has that resource, if so, just update the quantity
        for(i = 0; i < 20; i++) {
          if(pcbArray[processLocation].allocation.type[i] == request) {
            pcbArray[processLocation].allocation.quantity[i] += 1; 
            foundExisting = 1;
            break;
          }
          //Set the openSpace var to the first available openSpace
          else {
            if(openSpace == -1) {
              openSpace = i;
            } 
          }
        }
        //Otherwise, give it a new resource
        if(!foundExisting) {
          pcbArray[processLocation].allocation.type[openSpace] = request;
          pcbArray[processLocation].allocation.quantity[openSpace] += 1; 
        }
      } 
    }
    //If the release flag was set, release on of the selected resources
    //and put it back in the resource array
    if(release > -1) {
      pcbArray[processLocation].allocation.quantity[release] -= 1;
      resourceArray[release].quantAvail += 1;
      //If that was the last resource allocated to the process
      //set the type to -1
      if(pcbArray[processLocation].allocation.quantity[release] == 0) {
         pcbArray[processLocation].allocation.type[release] = -1;
      }
    }
  }
  //If the processID is 0, then it is no longer running. Proceed with cleanup
  else {
    printf("Process completed its time\n");
    fprintf(file, "Process completed its time\n");
    int i;
    int resource;
    //Go through all the allocations to the dead process and put them back into the
    //resource array
    for(i = 0; i < 20; i++) {
      if((resource = pcbArray[processLocation].allocation.type[i]) != -1) {
        resourceArray[resource].quantAvail += pcbArray[processLocation].allocation.quantity[i];
        pcbArray[processLocation].allocation.type[i] = -1;
        pcbArray[processLocation].allocation.quantity[i] = 0;
      } 
    }
    updateAverageTT(processLocation);
    pcbArray[processLocation].totalTimeRan = 0;
    pcbArray[processLocation].createTime = 0;
    pcbArray[processLocation].request = -1;
    pcbArray[processLocation].release = -1;
  }

}

/**
 * Author: Jason Klamert
 * Date: 11/26/2016
 * Description: initialize resources randomly.
 **/
void setupResources(void) {
  int i;
  for(i = 0; i < 20; i++) {
    resourceArray[i].type = i; 
    resourceArray[i].quantity = 1 + rand() % 10;
    resourceArray[i].quantAvail = resourceArray[i].quantity;
  }

  int numShared = 3 + rand() % 3;

  for(i = 0; i < numShared; i++) {
    int choice = rand() % 20;
    resourceArray[choice].quantity = 9999;
    resourceArray[choice].quantAvail = 9999;
  }
}

/**
 * Author: Jason Klamert
 * Date: 11/26/2016
 * Description: Interrupt Handler
 **/
void interruptHandler(int SIG){
  signal(SIGQUIT, SIG_IGN);
  signal(SIGINT, SIG_IGN);

  if(SIG == SIGINT) {
    fprintf(stderr, "CTRL-C received. Terminating\n");
  }

  if(SIG == SIGALRM) {
    fprintf(stderr, "oss: has timed out. Terminating.\n");
  }

  if(!cleanupCalled) {
    cleanupCalled = 1;
    cleanup();
  }
}

/**
 * Author: Jason Klamert
 * Date: 11/26/2016
 * Description: Cleans up the environment.
 **/
void cleanup() {

  signal(SIGQUIT, SIG_IGN);
  myStruct->sigNotReceived = 0;

  printf("oss: sending SIGQUIT\n");
  kill(-getpgrp(), SIGQUIT);

  //free up the malloc'd memory for the arguments
  free(mArg);
  free(nArg);
  free(pArg);
  free(rArg);
  free(tArg);
  printf("oss: waiting on all processes to die\n");
  childPid = wait(&status);

  //Detach and remove the shared memory after all child process have died
  if(RemoveTimer(shmid, myStruct) == -1) {
    perror("Failed to destroy shared messageQ shared mem seg");
  }

  if(RemoveArray(pcbShmid, pcbArray) == -1) {
    perror("Failed to destroy shared pcb shared mem seg");
  }

  if(RemoveResource(resourceShmid, resourceArray) == -1) {
    perror("Faild to destroy resource shared mem seg");
  }

  printf("oss: about to delete message queues\n");
  msgctl(ossQueueId, IPC_RMID, NULL);

  if(fclose(file)) {
    perror("Error closing file");
  }
  printf("oss about to terminate\n");

  printf("\n\n");
  printf("           STATISTICS:                  \n");
  printf("-----------------------------------------\n");
  printf("CPU IDLE TIME:\n");
  printf("%llu.%09llu\n", idleTime / NANO_MODIFIER, idleTime % NANO_MODIFIER);
  printf("\n");
  printf("AVG TURNAROUND:\n");
  printf("%llu.%09llu\n", (totalProcessLifeTime / totalProcessesSpawned) / NANO_MODIFIER, (totalProcessLifeTime / totalProcessesSpawned) % NANO_MODIFIER);
  printf("\n");
  printf("AVG WAIT:\n");
  printf("%llu.%09llu\n",(processWaitTime / totalProcessesSpawned) / NANO_MODIFIER, (processWaitTime / totalProcessesSpawned) % NANO_MODIFIER);
    printf("-----------------------------------------\n");

  printf("\n\n");
 
  exit(1);
}


//Detach and remove function
int RemoveTimer(int shmid, sharedStruct *shmaddr) {
  printf("oss: Detach and Remove Shared Memory\n");
  int error = 0;
  if(shmdt(shmaddr) == -1) {
    error = errno;
  }
  if((shmctl(shmid, IPC_RMID, NULL) == -1) && !error) {
    error = errno;
  }
  if(!error) {
    return 0;
  }

  return -1;
}

//Detach and remove function
int RemoveArray(int shmid, PCB *shmaddr) {
  printf("oss: Detach and Remove Shared Memory\n");
  int error = 0;
  if(shmdt(shmaddr) == -1) {
    error = errno;
  }
  if((shmctl(shmid, IPC_RMID, NULL) == -1) && !error) {
    error = errno;
  }
  if(!error) {
    return 0;
  }

  return -1;
}

//Detach and remove function
int RemoveResource(int shmid, resource *shmaddr) {
  printf("oss: Detach and Remove Shared Memory\n");
  int error = 0;
  if(shmdt(shmaddr) == -1) {
    error = errno;
  }
  if((shmctl(shmid, IPC_RMID, NULL) == -1) && !error) {
    error = errno;
  }
  if(!error) {
    return 0;
  }

  return -1;
}

//short help message
void printHelpMessage(void) {
  printf("\nAcceptable options are:\n");
  printf("[-h], [-help], [-l][fileName], [-s][numberusers], [-t][timeInSeconds]\n\n");
}
