// protect from multiple compiling
#ifndef OS_SIM_DRIVER_H
#define OS_SIM_DRIVER_H

// header files
#include <stdio.h>
#include "datatypes.h"
#include "StringUtils.h"
#include "configops.h"
#include "metadataops.h"
#include "simulator.h"

// program constants
typedef enum {
    MIN_NUM_ARGS = 3,
    LAST_FOUR_LETTERS = 4,
} PRGRM_CONSTANTS;

// Command line struct for storing command line switch settings
typedef struct CmdLineDataStruct {
	bool programRunFlag;
	bool configDisplayFlag;
	bool mdDisplayFlag;
	bool runSimFlag;
	char fileName[STD_STR_LEN];
} CmdLineData;

// prototypes
void clearCmdLineStruct(CmdLineData *clDataPtr);

bool processCmdLine(int numArgs, char **strVector, CmdLineData *clDataPtr);

void showCommandLineFormat();

#endif // OS_SIM_DRIVER_H