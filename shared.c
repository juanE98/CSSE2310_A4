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
#include "shared.h" 

#define FUNCTION_BUFFER 100
#define JOBDETAILS_BUFFER 255

/**
 * Removes newline character at the end of the word. 
 */ 
char* remove_newline(char* word) {
    size_t len = strlen(word) - 1; 
    if (word[len] == '\n') {
        word[len] = '\0'; 
    }
    return word; 
}

/**
 * Returns true if input is a number, false otherwise. 
 */ 
bool is_number(char* port) {
    int i; 
    for (i = 0; port[i] != '\0'; i++) {
        if (isdigit(port[i]) == 0) {
            return false; 
        }
    }
    return true;  
}

/**
 * Function that concantenates a given char** (str1) to a 
 * char** (stringReturned). 
 */ 
void create_comma_strings(char** str1, char** stringReturned) {
    strcat(*stringReturned, *str1); 
    strcat(*stringReturned, ", ");  
}

/** 
 * Parses through line and created a JobFile struct to populate fields and
 * returns it.
 *
 * @param jobDetails
 * @param newJob
 */
JobFile parse_jobdetails(char* jobDetails, JobFile newJob) {
    char details[JOBDETAILS_BUFFER];
    char function[FUNCTION_BUFFER];
    int functionCount;
    double lower;
    int lowerCount;
    double upper;
    int upperCount;
    int segments;
    int segmentsCount;
    int threads;
    int threadsCount;
    
    strcpy(details, jobDetails);
    sscanf(details, "%[^,]%n,%lf%n,%lf%n,%d%n,%d%n", function, &functionCount,
            &lower, &lowerCount, &upper, &upperCount, &segments,
            &segmentsCount, &threads, &threadsCount);

    newJob.function = function;
    newJob.lower = lower;
    newJob.upper = upper;
    newJob.segments = segments;
    newJob.threads = threads;
     
    return newJob;
}

/**
 * Validates a specific jobfile for errors. Returns error code if any errors 
 * are found 
 *
 * @param jobDetails job details (comma separated) 
 * @param lineNum corresponding line number in file 
 */ 
JobError validate_jobfile(char* jobDetails, int lineNum) {
    remove_newline(jobDetails); 
    JobFile job = { NULL, 0, 0, 0, 0};
    job = parse_jobdetails(jobDetails, job);

    for (int i = 0; i < (int)strlen(job.function); i++) {
        if (isspace(job.function[i]) != 0) {
            return WHITESPACE_DETECT; 
        }
    }

    if (job.upper <= job.lower) {
        return UPPER_LESS; 
    } else if (job.segments <= 0) {
        return SEGMENTS_NEGATIVE; 
    } else if (job.threads <= 0) {
        return THREADS_NEGATIVE; 
    } else if (job.segments % job.threads != 0) {
        return SEGMENTS_PER_THREAD; 
    }

    return NO_ERRORS; 
}
