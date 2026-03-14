// header files
#include "simulator.h"

// function implementations
bool accessMemory(MemoryManager *mmu, int pid, int base, 
                  int offset, char *message)
{
    int end = base + offset - 1;
    
    MemBlock *current = mmu->memListHead;

    while (current != NULL)
    {
        if (current->pid == pid)
        {
            if (base >= current->logicalBase && end <= current->logicalLimit)
            {
                sprintf(message, "After access success\n");
                
                return true;
            }
        }

        current = current->next;
    }

    sprintf(message, "After access failure\n");
    
    return false;
}

bool allocateMemory(MemoryManager *mmu, int pid, int base,
                    int offset, char *message)
{
    int size = offset;
    
    int available;
    
    MemBlock *current = mmu->memListHead;
    
    MemBlock *newBlock;

    while (current != NULL)
    {    
        if (current->pid == -1)
        {
            available = current->endAddr - current->startAddr + 1;

            if (available >= size)
            {
                current->pid = pid;
    
                current->logicalBase = base;
    
                current->logicalLimit = base + offset - 1;

                if (available > size)
                {
                    newBlock = (MemBlock *) malloc(sizeof(MemBlock));

                    if (newBlock != NULL)
                    {
                        newBlock->startAddr =
                            current->startAddr + size;
                   
                        newBlock->endAddr = current->endAddr;
                   
                        newBlock->pid = -1;
                   
                        newBlock->logicalBase = 0;
                   
                        newBlock->logicalLimit = 0;
                   
                        newBlock->next = current->next;

                        current->endAddr =
                            current->startAddr + size - 1;
                   
                        current->next = newBlock;
                    }
                }

                sprintf(message, "After allocate success\n");

                return true;
            }
        }

        current = current->next;
    }

    sprintf(message, "After allocate overlap failure\n");

    return false;
}

bool appendLog(LogBuffer *buffer, const char *line,
               ConfigDataType *configPtr, pthread_mutex_t *logMutex)
{
    int index;
    
    int oldCapacity;
    
    int newCapacity;
    
    int textLength;
    
    char *lineCopy;
    
    char **expandedLines;

    if (line == NULL || configPtr == NULL)
    {
        return false;
    }

    if (pthread_mutex_lock(logMutex) != 0)
    {
        return false;
    }

    if (buffer != NULL && buffer->lines == NULL)
    {
        if (CAPACITY_LEN > 0)
        {
            buffer->capacity = CAPACITY_LEN;
        }
    
        else
        {
            buffer->capacity = STD_STR_LEN;
        }

        buffer->count = 0;

        buffer->lines =
            (char **) malloc(buffer->capacity * sizeof(char *));
    
        if (buffer->lines == NULL)
        {
            pthread_mutex_unlock(logMutex);
    
            return false;
        }

        for (index = 0; index < buffer->capacity; index++)
        {
            buffer->lines[index] = NULL;
        }
    }

    if (buffer != NULL && buffer->lines != NULL)
    {
        if (buffer->count >= buffer->capacity)
        {
            oldCapacity = buffer->capacity;
    
            newCapacity = oldCapacity * 2;

            expandedLines =
                (char **) realloc(buffer->lines,
                                  newCapacity * sizeof(char *));
    
            if (expandedLines == NULL)
            {
                pthread_mutex_unlock(logMutex);
    
                return false;
            }

            buffer->lines = expandedLines;

            for (index = oldCapacity; index < newCapacity; index++)
            {
                buffer->lines[index] = NULL;
            }

            buffer->capacity = newCapacity;
        }

        textLength = getStringLength(line);

        lineCopy = (char *) malloc((textLength + 1) * sizeof(char));
    
        if (lineCopy == NULL)
        {
            pthread_mutex_unlock(logMutex);
    
            return false;
        }

        copyString(lineCopy, line);

        buffer->lines[buffer->count] = lineCopy;
    
        buffer->count++;
    }

    pthread_mutex_unlock(logMutex);

    return true;
}

void clearLogBuffer(LogBuffer *buffer)
{
    int lineIndex;

    if (buffer != NULL)
    {
        if (buffer->lines != NULL)
        {
            for (lineIndex = 0;
                 lineIndex < buffer->count;
                 lineIndex++)
            {
                if (buffer->lines[lineIndex] != NULL)
                {
                    free(buffer->lines[lineIndex]);
                    
                    buffer->lines[lineIndex] = NULL;
                }
            }

            free(buffer->lines);
            
            buffer->lines = NULL;
        }

        buffer->count = 0;
        
        buffer->capacity = 0;
    }
}

void clearMemory(MemoryManager *mmu, int pid, char *message)
{
    MemBlock *currentBlock = mmu->memListHead;

    while (currentBlock != NULL)
    {
        if (pid == -1 || currentBlock->pid == pid)
        {
            currentBlock->pid = -1;
            
            currentBlock->logicalBase = 0;
            
            currentBlock->logicalLimit = 0;
        }

        currentBlock = currentBlock->next;
    }

    if (pid == -1)
    {
        sprintf(message, "After clear process success\n");
    }

    else
    {
        sprintf(message, "After clear process %d success\n", pid);
    }
}

PCB *clearPCBList(PCB *pcbHead)
{
    PCB *currentPcb = pcbHead;
    
    PCB *nextPcb = NULL;

    while (currentPcb != NULL)
    {
        nextPcb = currentPcb->next;
    
        free(currentPcb);
    
        currentPcb = nextPcb;
    }

    return NULL;
}

bool createProcessControlBlocks(OpCodeType *metaDataMstrPtr, 
                                ConfigDataType *configPtr, PCB **pcbHead)
{
    if (metaDataMstrPtr == NULL || pcbHead == NULL)
    {
        return false;
    }

    *pcbHead = NULL;
    
    PCB *tail = NULL;

    PCB *newPcb;
    
    OpCodeType *mdPtr = metaDataMstrPtr;

    OpCodeType *scanner;

    bool endFound;
    
    int nextPid = 0;

    while (mdPtr != NULL)
    {
        if (compareString(mdPtr->command, "app") == STR_EQ &&
            compareString(mdPtr->strArg1, "start") == STR_EQ)
        {
            nextPid++;
            
            newPcb = (PCB *)malloc(sizeof(PCB));
            
            if (newPcb == NULL)
            {
                *pcbHead = clearPCBList(*pcbHead);
            
                return false;
            }

            newPcb->pid = nextPid;
            
            newPcb->state = READY_STATE;
            
            newPcb->opListHead = mdPtr;
            
            newPcb->currentOp = mdPtr->nextNode;
            
            newPcb->totalCpuCycles = 0;
            
            newPcb->remainingCycles = 0;

            newPcb->totalTimeMs = 0;

            newPcb->remainingTimeMs = 0;
            
            newPcb->next = NULL;
            
            newPcb->readyNext = NULL;

            if (*pcbHead == NULL)
            {
                *pcbHead = newPcb;

                tail = newPcb;
            }

            else
            {
                tail->next = newPcb;

                tail = newPcb;
            }

            scanner = mdPtr->nextNode;

            endFound = false;

            while (scanner != NULL && !endFound)
            {
                scanner->pid = newPcb->pid;

                if (compareString(scanner->command, "cpu") == STR_EQ &&
                    compareString(scanner->strArg1, "process") == STR_EQ)
                {
                    newPcb->totalCpuCycles += scanner->intArg2;

                    newPcb->totalTimeMs += 
                                   scanner->intArg2 * configPtr->procCycleRate;
                }

                else if (compareString(scanner->command, "dev") == STR_EQ)
                {
                    newPcb->totalTimeMs += 
                                     scanner->intArg2 * configPtr->ioCycleRate;
                }

                else if (compareString(scanner->command, "app") == STR_EQ &&
                         compareString(scanner->strArg1, "end") == STR_EQ)
                {
                    endFound = true;
                }

                scanner = scanner->nextNode;
            }

            if (!endFound)
            {
                *pcbHead = clearPCBList(*pcbHead);
                
                return false;
            }

            newPcb->remainingCycles = newPcb->totalCpuCycles;

            newPcb->remainingTimeMs = newPcb->totalTimeMs;

            mdPtr = scanner;
        }

        else
        {
            mdPtr = mdPtr->nextNode;
        }
    }

    return true;
}

static InterruptNode *dequeueAllInterrupts(Simulation *sim)
{
    InterruptNode *headCopy;

    pthread_mutex_lock(&sim->interruptMutex);

    headCopy = sim->interruptHead;

    sim->interruptHead = NULL;
    
    sim->interruptTail = NULL;

    pthread_mutex_unlock(&sim->interruptMutex);

    return headCopy;
}

static PCB *dequeueReady(Simulation *sim)
{
    PCB *headPcb;

    if (sim == NULL)
    {
        return NULL;
    }

    headPcb = sim->readyHead;

    if (headPcb != NULL)
    {
        sim->readyHead = headPcb->readyNext;

        if (sim->readyHead == NULL)
        {
            sim->readyTail = NULL;
        }

        headPcb->readyNext = NULL;
    }

    return headPcb;
}

static void destroySimulationContext(Simulation *sim)
{
    InterruptNode *current;
    
    InterruptNode *nextNode;

    pthread_mutex_lock(&sim->interruptMutex);

    current = sim->interruptHead;
    
    while (current != NULL)
    {
        nextNode = current->next;
    
        free(current);
    
        current = nextNode;
    }

    sim->interruptHead = NULL;
    
    sim->interruptTail = NULL;
    
    sim->activeIoCount = 0;

    pthread_mutex_unlock(&sim->interruptMutex);

    pthread_mutex_destroy(&sim->interruptMutex);
    
    pthread_cond_destroy(&sim->interruptCond);
}

void displayMemoryLayout(MemoryManager *mmu, ConfigDataType *configPtr,
                         LogBuffer *logBuf, pthread_mutex_t *logMutex,
                         const char *header)
{
    char line[MAX_STR_LEN];
    
    MemBlock *currentBlock = mmu->memListHead;

    if (configPtr->memDisplay == ON)
    {
        writeLogToMonitor(configPtr,
            "\n--------------------------------------------------\n");
    
        writeLogToMonitor(configPtr, header);
    }

    appendLog(logBuf, "\n--------------------------------------------------\n",
              configPtr, logMutex);

    appendLog(logBuf, header, configPtr, logMutex);

    if (currentBlock == NULL)
    {
        if (configPtr->memDisplay == ON)
        {
            writeLogToMonitor(configPtr, "No memory configured\n");
        }

        appendLog(logBuf, "No memory configured\n",
                  configPtr, logMutex);
    }

    while (currentBlock != NULL)
    {
        if (currentBlock->pid == -1)
        {
            snprintf(line, sizeof(line),
                     "%d [ Open, P#: x, 0-0 ] %d\n",
                     currentBlock->startAddr,
                     currentBlock->endAddr);
        }

        else
        {
            snprintf(line, sizeof(line),
                     "%d [ Used, P#:%d, %d-%d ] %d\n",
                     currentBlock->startAddr,
                     currentBlock->pid,
                     currentBlock->logicalBase,
                     currentBlock->logicalLimit,
                     currentBlock->endAddr);
        }

        if (configPtr->memDisplay == ON)
        {
            writeLogToMonitor(configPtr, line);
        }

        appendLog(logBuf, line, configPtr, logMutex);

        currentBlock = currentBlock->next;
    }

    if (configPtr->memDisplay == ON)
    {
        writeLogToMonitor(configPtr,
            "--------------------------------------------------\n\n");
    }

    appendLog(logBuf,
              "--------------------------------------------------\n\n",
              configPtr, logMutex);
}

bool displayMemoryStatus(MemoryManager *mmu, int base, int offset, 
                         ConfigDataType *configPtr, LogBuffer *logBuf, 
                         pthread_mutex_t *logMutex, int pid)
{
    char message[MAX_STR_LEN];
    
    bool success = false;

    if (compareString(configPtr->lastMemCommand, "allocate") == STR_EQ)
    {
        success = allocateMemory(mmu, pid, base, offset, message);
        
        displayMemoryLayout(mmu, configPtr, logBuf, logMutex, message);
    }

    else if (compareString(configPtr->lastMemCommand, "access") == STR_EQ)
    {
        success = accessMemory(mmu, pid, base, offset, message);
    
        displayMemoryLayout(mmu, configPtr, logBuf, logMutex, message);
    }
    
    else if (compareString(configPtr->lastMemCommand, "clear") == STR_EQ)
    {
        clearMemory(mmu, pid, message);
    
        displayMemoryLayout(mmu, configPtr, logBuf, logMutex, message);
    }

    appendLog(logBuf, message, configPtr, logMutex);

    return success;
}

static void enqueueInterrupt(Simulation *sim, int pid, OpCodeType *operation)
{
    InterruptNode *interruptNode;

    interruptNode = malloc(sizeof(InterruptNode));

    if (interruptNode != NULL)
    {
        interruptNode->pid = pid;
        
        interruptNode->operation = operation;
        
        interruptNode->next = NULL;

        pthread_mutex_lock(&sim->interruptMutex);

        if (sim->interruptTail == NULL)
        {
            sim->interruptHead = interruptNode;
            
            sim->interruptTail = interruptNode;
        }
        else
        {
            sim->interruptTail->next = interruptNode;
            
            sim->interruptTail = interruptNode;
        }

        if (sim->activeIoCount > 0)
        {
            sim->activeIoCount--;
        }

        /*
         * Called from an I/O worker thread when a device operation finishes.
         * It adds an interrupt record to the shared list and signals the 
         * condition variable so the main simulator thread knows an I/O event
         * is ready to be handled.
         */
        pthread_cond_signal(&sim->interruptCond);

        pthread_mutex_unlock(&sim->interruptMutex);
    }
}

static void enqueueReady(Simulation *sim, PCB *pcb)
{
    PCB *prevPcb;
    
    PCB *currPcb;

    if (sim == NULL || pcb == NULL)
    {
        return;
    }

    pcb->readyNext = NULL;

    if (sim->configPtr != NULL &&
        sim->configPtr->cpuSchedCode == CPU_SCHED_RR_P_CODE)
    {
        if (sim->readyTail == NULL)
        {
            sim->readyHead = pcb;
            
            sim->readyTail = pcb;
        }
        
        else
        {
            sim->readyTail->readyNext = pcb;
            
            sim->readyTail = pcb;
        }
    }

    else if (sim->configPtr != NULL &&
             sim->configPtr->cpuSchedCode == CPU_SCHED_FCFS_P_CODE)
    {
        prevPcb = NULL;
       
        currPcb = sim->readyHead;

        while (currPcb != NULL && currPcb->pid < pcb->pid)
        {
            prevPcb = currPcb;
           
            currPcb = currPcb->readyNext;
        }

        if (prevPcb == NULL)
        {
            pcb->readyNext = sim->readyHead;
            
            sim->readyHead = pcb;
        }
        
        else
        {
            pcb->readyNext = currPcb;
            
            prevPcb->readyNext = pcb;
        }

        if (pcb->readyNext == NULL)
        {
            sim->readyTail = pcb;
        }

        if (sim->readyTail == NULL)
        {
            sim->readyTail = pcb;
        }
    }
}

static void handleIoCompletion(Simulation *sim, InterruptNode *node)
{
    PCB *pcb;
    
    char timeStr[STD_STR_LEN];
    
    char line[MAX_STR_LEN];
    
    bool parametersValid;
    
    bool pcbFound;

    parametersValid = true;
    
    if (sim == NULL || node == NULL)
    {
        parametersValid = false;
    }

    pcbFound = false;
    
    pcb = NULL;

    if (parametersValid)
    {
        pcb = sim->pcbHead;
    
        while (pcb != NULL && !pcbFound)
        {
            if (pcb->pid == node->pid)
            {
                pcbFound = true;
            }
    
            else
            {
                pcb = pcb->next;
            }
        }

        if (pcbFound)
        {
            /*
             * Runs in the main simulator thread. Given one interrupt record,
             * it finds the matching PCB, logs the end of the device operation,
             * and moves the process back to the READY state so the scheduler 
             * can run it again.
             */
            accessTimer(LAP_TIMER, timeStr);

            sprintf(line,
                    "%s, Process %d: end %s on %s\n\n",
                    timeStr,
                    pcb->pid,
                    node->operation->inOutArg,
                    node->operation->strArg1);

            writeLogToMonitor(sim->configPtr, line);
           
            appendLog(sim->logBuf, line, sim->configPtr, sim->logMutex);

            pcb->state = READY_STATE;

            if (sim->configPtr != NULL &&
                (sim->configPtr->cpuSchedCode == CPU_SCHED_FCFS_P_CODE ||
                 sim->configPtr->cpuSchedCode == CPU_SCHED_RR_P_CODE))
            {
                enqueueReady(sim, pcb);
            }

            accessTimer(LAP_TIMER, timeStr);

            sprintf(line,
                    "%s, OS: Process %d set back to READY state\n",
                    timeStr,
                    pcb->pid);

            writeLogToMonitor(sim->configPtr, line);
           
            appendLog(sim->logBuf, line, sim->configPtr, sim->logMutex);
        }
    }
}

void *ioThreadFunction(void *args)
{
    IOThreadArgs *ioArgs;
    
    Simulation *sim;
    
    int durationMs;
    
    int pid;
    
    OpCodeType *operation;
    
    bool validArgs;

    validArgs = true;
    
    if (args == NULL)
    {
        validArgs = false;
    }

    ioArgs = NULL;
    
    sim = NULL;
    
    durationMs = 0;
    
    pid = 0;
    
    operation = NULL;

    if (validArgs)
    {
        ioArgs = (IOThreadArgs *)args;
    
        sim = ioArgs->context;
    
        durationMs = ioArgs->durationMs;
    
        pid = ioArgs->pid;
    
        operation = ioArgs->operation;

        /*
         * Code for a detached I/O worker thread. It waits for the device time
         * by calling runTimer, then reports completion by calling 
         * enqueueInterrupt so the main simulator thread can later move the 
         * process back to READY.
         */
        runTimer(durationMs);
        
        enqueueInterrupt(sim, pid, operation);
        
        free(ioArgs);
    }

    pthread_exit(NULL);
    
    return NULL;
}

void initLogBuffer(LogBuffer *buffer)
{
    buffer->lines = NULL;

    buffer->count = 0;

    buffer->capacity = 0;
}

void initMemory(MemoryManager *mmu, int totalMem)
{
    MemBlock *block = (MemBlock *)malloc(sizeof(MemBlock));

    if (block != NULL)
    {
        mmu->memListHead = block;
        
        mmu->totalSystemMemory = totalMem;

        block->startAddr = 0;
        
        block->endAddr = totalMem - 1;
        
        block->pid = -1;
        
        block->logicalBase = 0;
        
        block->logicalLimit = 0;
        
        block->next = NULL;
    }
}

static void initSimulationContext(Simulation *sim, MemoryManager *mmu,
                                  PCB *pcbHead, ConfigDataType *configPtr,
                                  LogBuffer *logBuf, pthread_mutex_t *logMutex)
{
    sim->mmu = mmu;
    
    sim->pcbHead = pcbHead;
    
    sim->configPtr = configPtr;
    
    sim->logBuf = logBuf;
    
    sim->logMutex = logMutex;

    sim->interruptHead = NULL;
    
    sim->interruptTail = NULL;
    
    sim->activeIoCount = 0;
    
    sim->shutdownRequested = false;

    sim->readyHead = NULL;

    sim->readyTail = NULL;

    pthread_mutex_init(&sim->interruptMutex, NULL);
    
    pthread_cond_init(&sim->interruptCond, NULL);
}

static void processInterrupts(Simulation *sim)
{
    InterruptNode *interruptHead = NULL;
   
    InterruptNode *currentNode = NULL;
   
    InterruptNode *nextNode = NULL;
   
    bool validSim = true;

    if (sim == NULL)
    {
        validSim = false;
    }

    if (validSim)
    {
        interruptHead = dequeueAllInterrupts(sim);

        currentNode = interruptHead;
        
        while (currentNode != NULL)
        {
            nextNode = currentNode->next;

            /*
             * Called in the main simulator thread to pull any completed I/O
             * events from the interrupt list and move the affected processes
             * from BLOCKED back to READY so they can be scheduled again.
             */
            handleIoCompletion(sim, currentNode);

            free(currentNode);

            currentNode = nextNode;
        }
    }
}

static bool runCpuSlice(Simulation *sim, PCB *pcb, OpCodeType *cpuOp,
                        int maxDurationMs)
{
    char timeString[STD_STR_LEN];
    
    char logLine[MAX_STR_LEN];

    int cycleRateMs = 0;
    
    int totalCyclesForOp = 0;
    
    int fullBurstMs = 0;
    
    int cyclesToRun = 0;
    
    int sliceDurationMs = 0;
    
    int cyclesExecuted = 0;

    bool validPointers = true;
    
    bool preempted = false;

    PCB *scanPcb;

    if (sim == NULL || pcb == NULL || cpuOp == NULL)
    {
        validPointers = false;
    }

    if (validPointers)
    {
        cycleRateMs = sim->configPtr->procCycleRate;

        totalCyclesForOp = cpuOp->intArg2;

        fullBurstMs = totalCyclesForOp * cycleRateMs;

        if (maxDurationMs <= 0 || maxDurationMs >= fullBurstMs)
        {
            cyclesToRun = totalCyclesForOp;
            sliceDurationMs = fullBurstMs;
        }

        else
        {
            cyclesToRun = maxDurationMs / cycleRateMs;

            if (cyclesToRun <= 0)
            {
                cyclesToRun = 1;
            }

            if (cyclesToRun > totalCyclesForOp)
            {
                cyclesToRun = totalCyclesForOp;
            }

            sliceDurationMs = cyclesToRun * cycleRateMs;
        }

        accessTimer(LAP_TIMER, timeString);

        sprintf(logLine,
                "%s, Process %d: start CPU processing "
                "(up to %d cycles, max %d ms)\n",
                timeString, pcb->pid, cyclesToRun, sliceDurationMs);

        writeLogToMonitor(sim->configPtr, logLine);

        appendLog(sim->logBuf, logLine, sim->configPtr, sim->logMutex);

        cyclesExecuted = 0;

        preempted = false;

        while (cyclesExecuted < cyclesToRun &&
               cpuOp->intArg2 > 0 && !preempted)
        {
            runTimer(cycleRateMs);

            pcb->remainingCycles -= 1;
            
            pcb->remainingTimeMs -= cycleRateMs;

            if (pcb->remainingTimeMs < 0)
            {
                pcb->remainingTimeMs = 0;
            }

            cpuOp->intArg2 -= 1;

            if (cpuOp->intArg2 < 0)
            {
                cpuOp->intArg2 = 0;
            }

            cyclesExecuted++;

            processInterrupts(sim);

            if (sim->configPtr != NULL &&
                sim->configPtr->cpuSchedCode == CPU_SCHED_SRTF_P_CODE)
            {
                scanPcb = sim->pcbHead;

                while (scanPcb != NULL && !preempted)
                {
                    if ((scanPcb->state == NEW_STATE ||
                        scanPcb->state == READY_STATE) &&
                        scanPcb->remainingTimeMs < pcb->remainingTimeMs)
                    {
                        preempted = true;
                    }

                    else
                    {
                        scanPcb = scanPcb->next;
                    }
                }
            }
        }

        accessTimer(LAP_TIMER, timeString);

        sprintf(logLine,
                "%s, Process %d: end CPU processing (cycles %d)\n",
                timeString, pcb->pid, cyclesExecuted);

        writeLogToMonitor(sim->configPtr, logLine);
       
        appendLog(sim->logBuf, logLine, sim->configPtr, sim->logMutex);

        return (cpuOp->intArg2 == 0);
    }

    return true;
}

void runSim(ConfigDataType *configPtr, OpCodeType *metaDataMstrPtr)
{
    PCB *pcbHead = NULL;
    
    PCB *currentPcb = NULL;
    
    PCB *lastScheduled = NULL;
    
    PCB *scan = NULL;
    
    OpCodeType *currentOp = NULL;

    LogBuffer logBuffer;

    char timeStr[STD_STR_LEN];
    
    char line[MAX_STR_LEN];
    
    char clearMessage[MAX_STR_LEN];
    
    char finalMessage[MAX_STR_LEN];

    bool pcbCreated = false;
    
    bool memOk = false;
    
    bool processCompleted = false;
    
    bool keepRunning = true;
    
    bool allExit = true;
    
    bool haveCurrentPcb = false;
    
    bool keepRunningOps = false;
    
    bool advanceOp = false;
    
    bool isCpuOperation = false;
    
    bool isDeviceOperation = false;
    
    bool isMemoryOperation = false;

    int processorCycleTimeMs = 0;
    
    int cycles = 0;
    
    int durationMs = 0;
    
    int maxDurationMs = 0;
    
    int quantumCycles = 0;

    pthread_t ioThreadId;
    
    IOThreadArgs *ioArgs = NULL;

    MemoryManager mmu;
    
    Simulation sim;

    pthread_mutex_t logMutex;

    initLogBuffer(&logBuffer);
    
    pthread_mutex_init(&logMutex, NULL);

    pcbCreated = createProcessControlBlocks(metaDataMstrPtr,
                                            configPtr, &pcbHead);

    if (configPtr->logToCode == LOGTO_FILE_CODE)
    {
        printf("Simulator running for output to file only\n");

        appendLog(&logBuffer,
                  "Simulator running for output to file only\n",
                  configPtr, &logMutex);
    }

    if (!pcbCreated)
    {
        printf("Error: failed to create PCBs from metadata\n");

        appendLog(&logBuffer,
                  "Error: failed to create PCBs from metadata\n",
                  configPtr, &logMutex);
    }

    else
    {
        initMemory(&mmu, configPtr->memAvailable);

        initSimulationContext(&sim, &mmu, pcbHead, configPtr,
                              &logBuffer, &logMutex);

        if (configPtr->cpuSchedCode == CPU_SCHED_FCFS_P_CODE ||
            configPtr->cpuSchedCode == CPU_SCHED_RR_P_CODE)
        {
            scan = pcbHead;

            while (scan != NULL)
            {
                if (scan->state == READY_STATE)
                {
                    enqueueReady(&sim, scan);
                }

                scan = scan->next;
            }
        }

        displayMemoryLayout(&mmu, configPtr, &logBuffer, &logMutex,
                            "After memory initialization\n");

        accessTimer(ZERO_TIMER, timeStr);

        sprintf(line, "%s, OS: System Start\n", timeStr);

        writeLogToMonitor(configPtr, line);
        
        appendLog(&logBuffer, line, configPtr, &logMutex);

        processorCycleTimeMs = configPtr->procCycleRate;
        
        keepRunning = true;

        while (keepRunning)
        {
            allExit = true;
            
            scan = pcbHead;

            /*
             * Main simulator thread picks up any I/O completions that have
             * been queued by worker threads since the last check.
             */
            processInterrupts(&sim);

            while (scan != NULL && allExit)
            {
                if (scan->state != EXIT_STATE)
                {
                    allExit = false;
                }

                scan = scan->next;
            }

            if (allExit)
            {
                keepRunning = false;
            }

            else
            {
                if (configPtr->cpuSchedCode == CPU_SCHED_FCFS_P_CODE ||
                    configPtr->cpuSchedCode == CPU_SCHED_RR_P_CODE)
                {
                    currentPcb = dequeueReady(&sim);
                }

                else
                {
                    currentPcb = selectNextProcess(pcbHead, lastScheduled, 
                                                   configPtr);
                }

                if (currentPcb == NULL)
                {
                    /*
                     * No NEW/READY processes. At least one process is not
                     * EXIT, so they must all be BLOCKED on I/O. The main
                     * thread waits on the interrupt condition until a
                     * device completes and posts an interrupt.
                     */
                    accessTimer(LAP_TIMER, timeStr);

                    sprintf(line,
                            "%s, OS: CPU idle, waiting on I/O\n",
                            timeStr);

                    writeLogToMonitor(configPtr, line);
                    
                    appendLog(&logBuffer, line, configPtr, &logMutex);

                    pthread_mutex_lock(&sim.interruptMutex);

                    while (sim.activeIoCount > 0 &&
                           sim.interruptHead == NULL)
                    {
                        pthread_cond_wait(&sim.interruptCond,
                                          &sim.interruptMutex);
                    }

                    pthread_mutex_unlock(&sim.interruptMutex);
                    
                    haveCurrentPcb = false;
                }

                else
                {
                    haveCurrentPcb = true;
                }

                if (haveCurrentPcb)
                {
                    lastScheduled = currentPcb;

                    processCompleted = true;
                   
                    keepRunningOps = true;

                    accessTimer(LAP_TIMER, timeStr);

                    sprintf(line,
                            "%s, OS: Process %d selected with %d ms "
                            "remaining\n",
                            timeStr, currentPcb->pid,
                            currentPcb->remainingTimeMs);

                    writeLogToMonitor(configPtr, line);
                   
                    appendLog(&logBuffer, line,
                              configPtr, &logMutex);

                    currentPcb->state = READY_STATE;

                    accessTimer(LAP_TIMER, timeStr);

                    sprintf(line,
                            "%s, OS: Process %d set to READY state\n",
                            timeStr, currentPcb->pid);

                    writeLogToMonitor(configPtr, line);
                   
                    appendLog(&logBuffer, line,
                              configPtr, &logMutex);

                    currentPcb->state = RUNNING_STATE;

                    accessTimer(LAP_TIMER, timeStr);

                    sprintf(line,
                            "%s, OS: Process %d set to RUNNING state,"
                            " time remaining %d ms\n",
                            timeStr, currentPcb->pid,
                            currentPcb->remainingTimeMs);

                    writeLogToMonitor(configPtr, line);
                   
                    appendLog(&logBuffer, line,
                              configPtr, &logMutex);

                    currentOp = currentPcb->currentOp;

                    while (currentOp != NULL && keepRunningOps)
                    {
                        advanceOp = true;
                       
                        isCpuOperation = false;
                       
                        isDeviceOperation = false;
                       
                        isMemoryOperation = false;

                        if (currentOp->pid == currentPcb->pid)
                        {
                            if (compareString(currentOp->command, 
                                              "cpu") == STR_EQ &&
                                compareString(currentOp->strArg1,
                                              "process") == STR_EQ)
                            {
                                isCpuOperation = true;
                            }

                            else if (compareString(currentOp->command,
                                                   "dev") == STR_EQ)
                            {
                                isDeviceOperation = true;
                            }

                            else if (compareString(currentOp->command,
                                                   "mem") == STR_EQ)
                            {
                                isMemoryOperation = true;
                            }

                            if (isCpuOperation)
                            {
                                maxDurationMs = 0;
                                
                                quantumCycles = 0;

                                if (configPtr->cpuSchedCode ==
                                    CPU_SCHED_RR_P_CODE)
                                {
                                    quantumCycles = configPtr->quantumCycles;

                                    if (quantumCycles <= 0)
                                    {
                                        maxDurationMs = currentOp->intArg2 *
                                                        processorCycleTimeMs;
                                    }

                                    else
                                    {
                                        maxDurationMs = quantumCycles *
                                                        processorCycleTimeMs;
                                    }
                                }

                                else
                                {
                                    /*
                                     * FCFS-N, SJF-N, or SRTF-P use the full 
                                     * remaining burst. Any needed preemption
                                     * happens when the scheduler runs again
                                     * after this slice is complete.
                                     */
                                    maxDurationMs = currentOp->intArg2 *
                                                    processorCycleTimeMs;
                                }

                                if (!runCpuSlice(&sim, currentPcb,
                                                 currentOp, maxDurationMs))
                                {
                                    /*
                                     * CPU burst was not finished in this slice
                                     * (preemptive case). Leave the op in 
                                     * place, mark the PCB READY, and let the
                                     * scheduler pick a process.
                                     */
                                    advanceOp = false;
                                    
                                    processCompleted = false;
                                    
                                    keepRunningOps = false;
                                    
                                    currentPcb->state = READY_STATE;

                                    if (configPtr->cpuSchedCode ==
                                        CPU_SCHED_RR_P_CODE)
                                    {
                                        enqueueReady(&sim, currentPcb);
                                    }
                                }
                            }

                            if (isDeviceOperation && keepRunningOps)
                            {
                                cycles = currentOp->intArg2;

                                durationMs = cycles * configPtr->ioCycleRate;

                                accessTimer(LAP_TIMER, timeStr);

                                sprintf(line,
                                        "%s, Process %d: start %s on %s "
                                        "(duration %d ms)\n",
                                        timeStr, currentPcb->pid,
                                        currentOp->inOutArg,
                                        currentOp->strArg1, durationMs);

                                writeLogToMonitor(configPtr, line);
                                
                                appendLog(&logBuffer, line,
                                          configPtr, &logMutex);

                                ioArgs = malloc(sizeof(IOThreadArgs));

                                if (ioArgs != NULL)
                                {
                                    ioArgs->pid = currentPcb->pid;
                                    
                                    ioArgs->pcb = currentPcb;
                                    
                                    ioArgs->operation = currentOp;
                                    
                                    ioArgs->durationMs = durationMs;
                                    
                                    ioArgs->context = &sim;

                                    currentPcb->state = BLOCKED_STATE;

                                    accessTimer(LAP_TIMER, timeStr);

                                    sprintf(line,
                                            "%s, OS: Process %d set to "
                                            "BLOCKED state\n",
                                            timeStr, currentPcb->pid);

                                    writeLogToMonitor(configPtr, line);
                                   
                                    appendLog(&logBuffer, line,
                                              configPtr, &logMutex);

                                    pthread_mutex_lock(&sim.interruptMutex);
                                    
                                    sim.activeIoCount++;
                                    
                                    pthread_mutex_unlock(&sim.interruptMutex);

                                    /*
                                     * Start an I/O worker thread for this 
                                     * device operation. The worker waits for
                                     * durationMs using runTimer and then 
                                     * queues an interrupt so the main thread
                                     * can finish the device event and unblock
                                     * the process.
                                     */
                                    pthread_create(&ioThreadId, NULL,
                                                   ioThreadFunction, ioArgs);
                                   
                                    pthread_detach(ioThreadId);

                                    currentPcb->remainingTimeMs -= durationMs;

                                    if (currentPcb->remainingTimeMs < 0)
                                    {
                                        currentPcb->remainingTimeMs = 0;
                                    }

                                    processCompleted = false;
                                    
                                    currentOp = currentOp->nextNode;
                                    
                                    currentPcb->currentOp = currentOp;
                                    
                                    advanceOp = false;
                                    
                                    keepRunningOps = false;
                                }
                            }

                            if (isMemoryOperation && keepRunningOps)
                            {
                                accessTimer(LAP_TIMER, timeStr);

                                copyString(configPtr->lastMemCommand,
                                           currentOp->strArg1);

                                sprintf(line,
                                        "%s, Process %d: memory %s "
                                        "request (base %d, offset %d)\n",
                                        timeStr, currentPcb->pid,
                                        currentOp->strArg1, currentOp->intArg2,
                                        currentOp->intArg3);

                                writeLogToMonitor(configPtr, line);

                                appendLog(&logBuffer, line,
                                          configPtr, &logMutex);

                                processInterrupts(&sim);

                                memOk = displayMemoryStatus(&mmu,
                                                            currentOp->intArg2,
                                                            currentOp->intArg3,
                                                            configPtr,
                                                            &logBuffer,
                                                            &logMutex,
                                                            currentPcb->pid);

                                accessTimer(LAP_TIMER, timeStr);

                                if (compareString(configPtr->lastMemCommand,
                                                  "allocate") == STR_EQ)
                                {
                                    if (memOk)
                                    {
                                        sprintf(line,
                                                "%s, Process: %d, successful"
                                                " mem allocate request\n",
                                                timeStr, currentPcb->pid);
                                    }

                                    else
                                    {
                                        sprintf(line,
                                               "%s, Process: %d, failed "
                                               "mem allocate request\n",
                                               timeStr, currentPcb->pid);
                                    }
                                }

                                else if (compareString(configPtr->lastMemCommand,
                                                       "access") == STR_EQ)
                                {
                                    if (memOk)
                                    {
                                        sprintf(line,
                                                "%s, Process: %d, successful"
                                                " mem access request\n",
                                                timeStr, currentPcb->pid);
                                    }

                                    else
                                    {
                                        sprintf(line,
                                                "%s, Process: %d, failed "
                                                "mem access request\n",
                                                timeStr, currentPcb->pid);
                                    }
                                }

                                else if (compareString(configPtr->lastMemCommand,
                                                       "clear") == STR_EQ)
                                {
                                    sprintf(line,
                                            "%s, Process: %d, cleared "
                                            "from memory\n",
                                            timeStr, currentPcb->pid);
                                }

                                writeLogToMonitor(configPtr, line);
                                
                                appendLog(&logBuffer, line,
                                          configPtr, &logMutex);
                            }
                        }

                        if (advanceOp && keepRunningOps)
                        {
                            currentOp = currentOp->nextNode;
                            
                            currentPcb->currentOp = currentOp;
                        }
                    }

                    /*
                     * If all metadata for this PCB has been consumed and the
                     * process is neither BLOCKED nor preempted, it transitions
                     * to EXIT and is removed from memory.
                     */
                    if (processCompleted && currentPcb->currentOp == NULL)
                    {
                        clearMemory(&mmu, currentPcb->pid, clearMessage);

                        displayMemoryLayout(&mmu, configPtr, &logBuffer,
                                            &logMutex, clearMessage);

                        currentPcb->state = EXIT_STATE;

                        accessTimer(LAP_TIMER, timeStr);

                        sprintf(line, "%s, OS: Process %d set to EXIT state\n",
                                timeStr, currentPcb->pid);

                        writeLogToMonitor(configPtr, line);
                        
                        appendLog(&logBuffer, line, configPtr, &logMutex);
                    }
                }
            }
        }

        accessTimer(LAP_TIMER, timeStr);

        sprintf(line, "%s, OS: System End\n", timeStr);

        writeLogToMonitor(configPtr, line);

        appendLog(&logBuffer, line, configPtr, &logMutex);

        clearMemory(&mmu, -1, finalMessage);

        displayMemoryLayout(&mmu, configPtr, &logBuffer, &logMutex,
                            "After clear all process success\n");

        pcbHead = clearPCBList(pcbHead);

        destroySimulationContext(&sim);
    }

    if (configPtr->logToCode == LOGTO_FILE_CODE ||
        configPtr->logToCode == LOGTO_BOTH_CODE)
    {
        printf("\nWriting log to file: %s\n",
               configPtr->logToFileName);

        writeLogToFile(&logBuffer, configPtr->logToFileName);

        printf("File write complete.\n");
    }

    pthread_mutex_destroy(&logMutex);
    
    clearLogBuffer(&logBuffer);
}

PCB* selectNextProcess(PCB *pcbHead, PCB *lastProcess, 
                       ConfigDataType *configPtr)
{
    PCB *selectedProcess = NULL;
    
    PCB *scanProcess = NULL;
    
    PCB *startProcess = NULL;
    
    bool validHead = true;

    if (pcbHead == NULL)
    {
        validHead = false;
    }

    if (!validHead)
    {
        return NULL;
    }

    /* ----------------------------------------------
     * SCHEDULER: SJF-N (Non-preemptive)
     * Choose NEW or READY process with smallest totalCpuCycles
     * ---------------------------------------------- */
    if (configPtr != NULL &&
        configPtr->cpuSchedCode == CPU_SCHED_SJF_N_CODE)
    {
        scanProcess = pcbHead;
        
        while (scanProcess != NULL)
        {
            if (scanProcess->state == NEW_STATE ||
                scanProcess->state == READY_STATE)
            {
                if (selectedProcess == NULL ||
                    scanProcess->totalCpuCycles <= selectedProcess->totalCpuCycles)
                {
                    selectedProcess = scanProcess;
                }
            }

            scanProcess = scanProcess->next;
        }

        return selectedProcess;
    }

    /* ----------------------------------------------
     * SCHEDULER: SRTF-P (Preemptive)
     * Choose NEW or READY process with smallest remainingTimeMs
     * Tie-break by PID for deterministic selection
     * ---------------------------------------------- */
    if (configPtr != NULL &&
        configPtr->cpuSchedCode == CPU_SCHED_SRTF_P_CODE)
    {
        scanProcess = pcbHead;
        while (scanProcess != NULL)
        {
            if (scanProcess->state == NEW_STATE ||
                scanProcess->state == READY_STATE)
            {
                if (selectedProcess == NULL ||
                    scanProcess->remainingTimeMs < selectedProcess->remainingTimeMs ||
                    (scanProcess->remainingTimeMs == selectedProcess->remainingTimeMs &&
                    scanProcess->pid < selectedProcess->pid))
                {
                    selectedProcess = scanProcess;
                }
            }

            scanProcess = scanProcess->next;
        }

        return selectedProcess;
    }

    /* ----------------------------------------------
     * DEFAULT: FCFS-N and RR-P circular selection
     * RR-P becomes preemptive due to CPU slicing,
     * not because of different selection logic.
     * ---------------------------------------------- */

    if (lastProcess != NULL)
    {
        startProcess = lastProcess->next;
    }

    else
    {
        startProcess = pcbHead;
    }

    scanProcess = startProcess;

    while (scanProcess != NULL)
    {
        if (scanProcess->state == NEW_STATE ||
            scanProcess->state == READY_STATE)
        {
            return scanProcess;
        }

        scanProcess = scanProcess->next;
    }

    scanProcess = pcbHead;

    while (scanProcess != NULL && scanProcess != startProcess)
    {
        if (scanProcess->state == NEW_STATE ||
            scanProcess->state == READY_STATE)
        {
            return scanProcess;
        }

        scanProcess = scanProcess->next;
    }

    return NULL;
}

void writeLogToFile(LogBuffer *buffer, const char *fileName)
{
    FILE *file;
    
    int lineIndex;

    file = fopen(fileName, "w");

    if(file != NULL)
    {
        for(lineIndex = 0; 
            lineIndex < buffer->count; 
            lineIndex = lineIndex + 1)
        {
            if(buffer->lines[lineIndex] != NULL)
            {
                fprintf(file, "%s", buffer->lines[lineIndex]);
            }
        }

        fclose(file);
    }
}

void writeLogToMonitor(const ConfigDataType *configPtr, const char *line)
{
    if (configPtr != NULL && line != NULL)
    {
        if (configPtr->logToCode == LOGTO_MONITOR_CODE ||
            configPtr->logToCode == LOGTO_BOTH_CODE)
        {
            fputs(line, stdout);

            fflush(stdout);
        }
    }   
}