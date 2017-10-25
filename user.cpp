#include "user.h"

int main (int argc, char **argv) {
  int timeoutValue = 30;
  long long startTime;
  long long currentTime;
  key_t ossKey = 128464;
  key_t userKey = 120314;

  int shmid = 0;
  int pcbShmid = 0;
  int resourceShmid = 0;

  char *fileName;
  char *defaultFileName = "logfile.txt";
  char *option = NULL;
  FILE *file;
  int c;
  myPid = getpid();

  while((c = getopt(argc, argv, "l:m:n:p:r:t:")) != -1)
    switch (c) {
      case 'l':
        fileName = optarg;
        break;
      case 'm':
        shmid = atoi(optarg);
        break;
      case 'n':
        processNumber = atoi(optarg);
        break;
      case 'p':
        pcbShmid = atoi(optarg);
        break;
      case 'r':
        resourceShmid = atoi(optarg);
        break;
      case 't':
        timeoutValue = atoi(optarg) + 2;
        break;
      case '?':
        fprintf(stderr, "Invalid arguements! Terminating.\n", myPid);
        exit(-1);
    }

  srand(time(NULL) + processNumber);

  if((myStruct = (sharedStruct *)shmat(shmid, NULL, 0)) == (void *) -1) {
    perror("user could not attach shared mem");
    exit(1);
  }

  if((pcbArray = (PCB *)shmat(pcbShmid, NULL, 0)) == (void *) -1) {
    perror("user could not attach to shared memory array");
    exit(1);
  }

  if((resourceArray = (resource *)shmat(resourceShmid, NULL, 0)) == (void *) -1) {
    perror("user could not attach to resource shared mem seg");
    exit(1);
  }

  signal(SIGINT, SIG_IGN);
  signal(SIGQUIT, sigquitHandler);
  signal(SIGALRM, killChildren);
  alarm(QUIT_TIMEOUT);

  if((ossQueueId = msgget(ossKey, IPC_CREAT | 0777)) == -1) {
    perror("user msgget for oss queue");
    exit(-1);
  }

  alarm(timeoutValue);

  int i = 0;
  int j;

  long long duration;
  int notFinished = 1;

  do {
  
    if(pcbArray[processNumber].request == -1) {
      if(willTerminate()) {
        notFinished = 0;
      }
      else {
        if(takeAction()) {
          int choice = rand() % 2;
          if(choice) {
            pcbArray[processNumber].request = chooseResource(); 
            sendMessage(ossQueueId, 3);
          }
          else {
            pcbArray[processNumber].release = 1;
            sendMessage(ossQueueId, 3);
          }
        }
      }
    }

  
  } while (notFinished && myStruct->sigNotReceived);

  pcbArray[processNumber].processID = 0;
  sendMessage(ossQueueId, 3);

  if(shmdt(myStruct) == -1) {
    perror("user could not detach shared memory struct");
  }

  if(shmdt(pcbArray) == -1) {
    perror("user could not detach from shared memory array");
  }

  if(shmdt(resourceArray) == -1) {
    perror("user could not detach from resource array");
  }

  printf("user %d exiting\n", processNumber);
  //exit(1);
  kill(myPid, SIGTERM);
  sleep(1);
  kill(myPid, SIGKILL);
  printf("user error\n");

}

/**
 * Author Jason Klamert
 * Date: 11/26/2016
 * Description: function to determine if it will terminate.
 **/
int willTerminate(void) {
  if(myStruct->ossTimer - pcbArray[processNumber].createTime >= NANO_MODIFIER) {
    int choice = 1 + rand() % 5;
    return choice == 1 ? 1 : 0;
  }
  return 0;
}

/**
 * Author Jason Klamert
 * Date: 11/26/2016
 * Description: function to choose a resource.
 **/
int chooseResource(void) {
  int choice = rand() % 20;
  return resourceArray[choice].type;
}

/**
 * Author Jason Klamert
 * Date: 11/26/2016
 * Description: function to choose a random action to take.
 **/
int takeAction(void) {
  int choice = rand() % 20;
  return choice == 1 ? 1 : 0;
}

/**
 * Author Jason Klamert
 * Date: 11/26/2016
 * Description: function to send message.
 **/
void sendMessage(int qid, int msgtype) {
  struct msgformat msg;

  msg.mType = msgtype;
  sprintf(msg.mText, "%d", processNumber);

  if(msgsnd(qid, (void *) &msg, sizeof(msg.mText), IPC_NOWAIT) == -1) {
    perror("user msgsnd error");
  }
}

/**
 * Author Jason Klamert
 * Date: 11/26/2016
 * Description: function to handle the quit.
 **/
void sigquitHandler(int sig) {
  printf("user %d has received signal %s (%d)\n", processNumber, strsignal(sig), sig);

  if(shmdt(myStruct) == -1) {
    perror("user could not detach shared memory");
  }

  if(shmdt(pcbArray) == -1) {
    perror("user could not detach from shared memory array");
  }

  if(shmdt(resourceArray) == -1) {
    perror("user could not detach from resource array");
  }

  kill(myPid, SIGKILL);

  //The users have at most 5 more seconds to exit gracefully or they will be SIGTERM'd
  alarm(5);
}

/**
 * Author Jason Klamert
 * Date: 11/26/2016
 * Description: function to kill the zombie children.
 **/
void killChildren(int signum) {
  printf("user %d is killing itself due to user timeout override\n", processNumber);
  kill(myPid, SIGTERM);
  sleep(1);
  kill(myPid, SIGKILL);
}
