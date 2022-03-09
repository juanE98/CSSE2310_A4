#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>           
#include <limits.h>
#include <netdb.h>
#include <ctype.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>


#ifndef SHARED_H 
#define SHARED_H

typedef struct {                                                            
    char* function;
    double lower;                                                           
    double upper;                        
    int segments;
    int threads;
} JobFile;

typedef enum {
    NO_ERRORS = 0,
    WHITESPACE_DETECT = 1,
    UPPER_LESS = 2,
    SEGMENTS_NEGATIVE = 3,
    THREADS_NEGATIVE = 4,
    SEGMENTS_PER_THREAD = 5,
    BAD_EXPRESSION = 6
} JobError;

char* remove_newline(char* word);

bool is_number(char* port);

void create_comma_strings(char** str1, char** stringReturned);

JobFile parse_jobdetails(char* jobDetails, JobFile newJob); 

JobError validate_jobfile(char* jobDetails, int lineNum); 

#endif
