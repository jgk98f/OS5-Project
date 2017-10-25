#ifndef USER_H
#define USER_H
#include <ctype.h>
#include <sys/wait.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/msg.h>
#include <signal.h>
#include <errno.h>
#include <time.h>
#include "resource.h"
int willTerminate(void);
void sendMessage(int, int);
int chooseResource(void);
int takeAction(void);
void alarmHandler(int);
void sigquitHandler(int);
void killChildren(int);
pid_t myPid;
long long *ossTimer;
struct sharedStruct *myStruct;
int processNumber = 0;
int ossQueueId;
const int QUIT_TIMEOUT = 10;
struct msqid_ds msqid_buf;
struct PCB *pcbArray;
resource *resourceArray;
#endif
