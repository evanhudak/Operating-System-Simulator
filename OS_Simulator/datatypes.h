// protect from multiple compiling
#ifndef DATATYPES_H
#define DATATYPES_H

// header files
#include <stdio.h>
#include <stdbool.h>
#include "StandardConstants.h"

// GLOBAL CONSTANTS - may be used in other files

typedef enum {
	CMD_STR_LEN = 5,
	IO_ARG_STR_LEN = 5,
	STR_ARG_LEN = 15
} OpCodeArrayCapacity;

// Five state data structure for process
typedef enum {
	NEW_STATE,
	READY_STATE,
	RUNNING_STATE,
	BLOCKED_STATE,
	EXIT_STATE
} ProcessState;

#endif // DATATYPES_H