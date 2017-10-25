#ifndef RESOURCE_H
#define RESOURCE_H
#include <sys/types.h>
#include <stdbool.h>

static const long long NANO_MODIFIER = 1000000000;
static const int INFINITE = -5;

typedef struct sharedStruct {
  long long ossTimer;
  int sigNotReceived;
  pid_t scheduledProcess;
} sharedStruct;

typedef struct msgformat {
  long mType;
  char mText[80];
} msgformat;

typedef struct resourceAlloc {
  int type[20];
  int quantity[20];
} resourceAlloc;

typedef struct PCB {
  pid_t processID;
  int request;
  int release;
  resourceAlloc allocation;
  long long totalTimeRan;
  long long createTime;
} PCB;

typedef struct resource {
  int type;
  int quantity;
  int quantAvail;
} resource;
#endif
