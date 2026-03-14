// Preprocessor directive
#ifndef STRING_UTILS_H
#define STRING_UTILS_H

// header files
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include "StandardConstants.h"

// prototypes
int compareString(const char *oneStr, const char *otherStr);

void concatenateString(char *destStr, const char *sourceStr);

void copyString(char *destStr, const char *sourceStr);

int findSubString(const char *testStr, const char *searchSubStr);

int getStringLength(const char *testStr);

bool getStringConstrained(FILE *inStream,
	                      bool clearLeadingNonPrintable,
	                      bool clearLeadingSpace,
	                      bool stopAtNonPrintable,
	                      char delimiter,
	                      char *capturedString);

bool getStringToDelimiter(FILE *inStream,
	                      char delimiter,
	                      char *capturedString);

bool getStringToLineEnd(FILE *inStream,
	                    char *capturedString);

void getSubString(char *destStr, const char *sourceStr,
	                   int startIndex, int endIndex);

void setStrToLowerCase(char *destStr, const char *sourceStr);

char toLowerCase(char testChar);

#endif // STRING_UTILS_H