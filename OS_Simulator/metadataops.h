// protect from multiple compiling
#ifndef META_DATA_OPS_H
#define META_DATA_OPS_H

// header files
#include <stdbool.h>
#include "datatypes.h"
#include "StandardConstants.h"
#include "StringUtils.h"

// GLOBAL CONSTANTS
typedef enum {
	BAD_ARG_VAL = -1,
	NO_ACCESS_ERR,
	MD_FILE_ACCESS_ERR,
	MD_CORRUPT_DESCRIPTOR_ERR,
	OPCMD_ACCESS_ERR,
	CORRUPT_OPCMD_ERR,
	CORRUPT_OPCMD_ARG_ERR,
	UNBALANCED_START_END_ERR,
	COMPLETE_OPCMD_FOUND_MSG,
	LAST_OPCMD_FOUND_MSG
} OpCodeMessages;

typedef struct OpCodeType {
    int pid;                          // pid, added when PCB is created
    char command[MAX_STR_LEN];      // three letter command quantity
    char inOutArg[STD_STR_LEN];  // for device in/out
    char strArg1[STD_STR_LEN];      // arg 1 descriptor, up to 12 chars
    int intArg2;                      // cycles or memory, assumes 4 byte int
    int intArg3;                      // memory, assumes 4 byte int
                                      //   also non/premption indicator
    double opEndTime;                 // size of time string returned from accessTimer
    struct OpCodeType *nextNode;      // pointer to next node as needed
} OpCodeType;

// function prototypes
OpCodeType *addNode(OpCodeType *localPtr, OpCodeType *newNode);

OpCodeType *clearMetaData(OpCodeType *localPtr);

void displayMetaData(const OpCodeType *localPtr);

int getCommand(char *cmd, const char *inputStr, int index);

bool getMetaData(const char *fileName,
	               OpCodeType **opCodeDataHead, char *endStateMsg);

int getNumberArg(int *number, const char *inputStr, int index);

OpCodeMessages getOpCommand(FILE *filePtr, OpCodeType *inData);

int getStringArg(char *strArg, const char *inputStr, int index);

bool isDigit(char testChar);

int updateEndCount(int count, const char *opString);

int updateStartCount(int count, const char *opString);

bool verifyFirstStringArg(const char *strArg);

bool verifyValidCommand(char *testCmd);

#endif // META_DATA_OPS_H