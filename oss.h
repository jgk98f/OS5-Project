#ifndef OSS_H
#define OSS_H
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/msg.h>
#include <errno.h>
#include <time.h>
#include <stdbool.h>
#include <ctype.h>
#include <sys/wait.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include "resource.h"


void spawnuser(void);
bool isTimeToSpawn(void);
void setTimeToSpawn(void);
int incrementTimer(void);
void updateAverageTT(int);
int processSystem(void);
void updateProcess(int);
void setupResources(void);
void interruptHandler(int);
void cleanup(void);
void sendMessage(int, int);
int RemoveTimer(int, sharedStruct*);
int RemoveArray(int, PCB*);
int RemoveResource(int, resource*);
void printHelpMessage(void);

//PCB Array//
PCB *pcbArray;
resource *resourceArray;

//Char arrays for arg passing to children//
char *mArg;
char *nArg;
char *pArg;
char *rArg;
char *tArg;

volatile sig_atomic_t cleanupCalled = 0;

pid_t myPid, childPid;
int tValue = 20;
int sValue = 3;
int status;
int shmid;
int pcbShmid;
int resourceShmid;
int userQueueId;
int ossQueueId;
int nextProcessToSend = 1;
int processNumberBeingSpawned = -1;
long long timeToSpawn = 0;
long long idleTime = 0;
long long turnaroundTime = 0;
long long processWaitTime = 0;
long long totalProcessLifeTime = 0;
int totalProcessesSpawned = 0;
int messageReceived = 0;
//long long *ossTimer = 0;

struct sharedStruct *myStruct;

//Constants for timing the program
const long long MAX_TIME = 20000000000;
const int MAX_FUTURE_SPAWN = 280000001;
const int MAX_IDLE_INCREMENT = 10001;
const int MAX_TOTAL_PROCESS_TIME = 700000001;
const int CHANCE_HIGH_PRIORITY = 20;
const int MAXuser = 20;
const int ARRAY_SIZE = 18;
FILE *file;
struct msqid_ds msqid_ds_buf;

key_t timerKey = 148364;
key_t pcbArrayKey = 135155;
key_t resourceKey = 131581;
key_t ossQueueKey = 128464;
#endif
