// protect from multiple compiling
#ifndef SIMULATOR_H
#define SIMULATOR_H

// header files
#include "datatypes.h"
#include "metadataops.h"
#include "configops.h"
#include "StringUtils.h"
#include "simtimer.h"
#include "StandardConstants.h"
#include <stdbool.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

// Forward Declaration
struct PCB;
struct Simulation;
struct InterruptNode;

// Global Constants
typedef struct InterruptNode {
	int pid;
	OpCodeType *operation;
	struct InterruptNode *next;
} InterruptNode;

typedef struct IOThreadArgs {
	int pid;
	int durationMs;
	struct PCB *pcb;
	OpCodeType *operation;
	struct Simulation *context;
} IOThreadArgs;

typedef struct LogBuffer {
	char **lines;
	int count;
	int capacity;
} LogBuffer;

typedef struct MemBlock {
    int startAddr;
    int endAddr;
    int pid;
    int logicalBase;
    int logicalLimit;
    struct MemBlock *next;
} MemBlock;

typedef struct MemoryManager {
    MemBlock *memListHead;
    int totalSystemMemory;
} MemoryManager;

typedef struct PCB {
	int pid;
	ProcessState state;
	OpCodeType *opListHead;
	OpCodeType *currentOp;
	int totalCpuCycles;
	int remainingCycles;
	int totalTimeMs;
	int remainingTimeMs;
	struct PCB *next;
	struct PCB *readyNext;
} PCB;

typedef struct Simulation {
    MemoryManager *mmu;
    PCB *pcbHead;
    ConfigDataType *configPtr;
    LogBuffer *logBuf;
    pthread_mutex_t *logMutex;
    InterruptNode *interruptHead;
    InterruptNode *interruptTail;
    int activeIoCount;
    bool shutdownRequested;
    PCB *readyHead;
    PCB *readyTail;
    pthread_mutex_t interruptMutex;
    pthread_cond_t interruptCond;
} Simulation;


// prototypes
bool accessMemory(MemoryManager *mmu, int pid, int base, 
                  int offset, char *message);

bool allocateMemory(MemoryManager *mmu, int pid, int base, 
                    int offset, char *message);

bool appendLog(LogBuffer *buffer, const char *line,
               ConfigDataType *configPtr, pthread_mutex_t *logMutex);

void clearLogBuffer(LogBuffer *buffer);

void clearMemory(MemoryManager *mmu, int pid, char *message);

PCB *clearPCBList(PCB *pcbHead);

bool createProcessControlBlocks(OpCodeType *metaDataMstrPtr, 
	                            ConfigDataType *configPtr, PCB **pcbHead);

static InterruptNode *dequeueAllInterrupts(Simulation *sim);

static PCB *dequeueReady(Simulation *sim);

static void destroySimulationContext(Simulation *sim);

void displayMemoryLayout(MemoryManager *mmu, ConfigDataType *configPtr, 
                         LogBuffer *logBuf, pthread_mutex_t *logMutex, 
                         const char *header);

bool displayMemoryStatus(MemoryManager *mmu, int base, int offset, 
                         ConfigDataType *configPtr, LogBuffer *logBuf, 
                         pthread_mutex_t *logMutex, int pid);

static void enqueueInterrupt(Simulation *sim, int pid, OpCodeType *operation);

static void enqueueReady(Simulation *sim, PCB *pcb);

static void handleIoCompletion(Simulation *sim, InterruptNode *node);

void *ioThreadFunction(void *args);

void initLogBuffer(LogBuffer *buffer);

void initMemory(MemoryManager *mmu, int totalMem);

static bool runCpuSlice(Simulation *sim, PCB *pcb, OpCodeType *cpuOp,
                        int maxDurationMs);

void runSim(ConfigDataType *configPtr, OpCodeType *metaDataMstrPtr);

PCB* selectNextProcess(PCB *pcbHead, PCB *lastProcess, 
                       ConfigDataType *configPtr);

void writeLogToFile(LogBuffer *buffer, const char *fileName);

void writeLogToMonitor(const ConfigDataType *configPtr, const char *line);

#endif // SIMULATOR_H