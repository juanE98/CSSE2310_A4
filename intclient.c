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

/* CSSE2310 Library */ 
#include <tinyexpr.h>
#include <csse2310a4.h>
#include <csse2310a3.h>

#define JOBFILE_ARGS 5
#define JOBDETAILS_BUFFER 255 
#define FUNCTION_BUFFER 100 
#define REQUEST_BUFFER 30
#define RESPONSE_BUFFER 1024 
#define MESSAGE_BUFFER 38 

/* Structure type to store arguments passed on the client*/ 
typedef struct {
    bool v; 
    char* portNum; 
    char* jobfile; 
} ClientArgs; 

/* Exit codes for program */ 
typedef enum {
    EXIT_NORMAL = 0, 
    INVALID_CMD = 1, 
    INVALID_PORTNUM = 2, 
    INVALID_COMM = 3, 
    INVALID_JOBFILE = 4
} ExitCode; 

/** Function prototypes */
ClientArgs process_cmd_args(int argc, char** argv); 
void request_integrate_function(char* function, double lowerVal, 
        double upperVal, int segmentsVal, int threadsVal, int serverFd);  
/****************************************************************************/ 

/**
 * Exits the program with given exit code. Prints error message to stderr 
 *
 * @param code The exit code that caused the program to exit.
 * @param portNum port number given 
 */ 
void exit_program(ExitCode code, char* portNum) {
    switch (code) {
        case EXIT_NORMAL:
            break; 
        case INVALID_CMD: 
            fprintf(stderr, "Usage: intclient [-v] portnum [jobfile]\n"); 
            break; 
        case INVALID_PORTNUM: 
            fprintf(stderr, "intclient: unable to connect to port %s\n", 
                    portNum);
            break; 
        case INVALID_COMM: 
            fprintf(stderr, "intclient: communication error\n"); 
            break; 
        default: 
            break; 
    }
    exit(code); 
}

/** 
 * Prints error messages to stderr for invalid jobfile line. 
 *
 * @param code Error detected 
 * @param lineNum line number in jobfile 
 * */ 
void jobfile_error(JobError code, int lineNum) {
    switch (code) {
        case NO_ERRORS: 
            break; 
        case WHITESPACE_DETECT: 
            fprintf(stderr, "intclient: spaces not permitted in expression" 
                    " (line %d)\n", lineNum); 
            break;
        case UPPER_LESS: 
            fprintf(stderr, "intclient: upper bound must be greater than" 
                    " lower bound (line %d)\n", lineNum);
            break; 
        case SEGMENTS_NEGATIVE: 
            fprintf(stderr, "intclient: segments must be a positive"
                    " integer (line %d)\n", lineNum);
            break; 
        case THREADS_NEGATIVE: 
            fprintf(stderr, "intclient: threads must be a positive" 
                    " integer (line %d)\n", lineNum);
            break; 
        case SEGMENTS_PER_THREAD: 
            fprintf(stderr, "intclient: segments must be an integer multiple"
                    " of threads (line %d)\n", lineNum); 
            break;
        default: 
            break; 
    }
}

/**
 * Connects to given host with specified port number. 
 */ 
int connect_to(const char* host, const char* port) {
    struct addrinfo* ai = NULL; 
    struct addrinfo hints; 

    memset(&hints, 0, sizeof(struct addrinfo)); 
    hints.ai_family = AF_INET;  
    hints.ai_socktype = SOCK_STREAM;
    int err;
    if ((err = getaddrinfo(host, port, &hints, &ai))) {
        freeaddrinfo(ai); 
        return INVALID_PORTNUM; 
    }

    //create socket 
    int fd = socket(AF_INET, SOCK_STREAM, 0); 
    //tries to connect to server 
    if (connect(fd, (struct sockaddr*)ai->ai_addr, sizeof(struct sockaddr))) {
        return INVALID_PORTNUM; 
    }
    // now connected to server 
    
    return fd; 
}

/**
 * Attempts to connect to intserver server on given port number (or service 
 * name) on localhost. 
 * If client is unable to connect to the port, error code 2 is emitted to
 * stderr. 
 *
 * @param port port number given 
 * 
 * @return ExitCode status 
 */ 
int connect_to_server(char* port) {
    int serverFd = connect_to(NULL, port); 
    if (serverFd == INVALID_PORTNUM) {
        exit_program(INVALID_PORTNUM, port); 
    }
    return serverFd; 
}

/**
 * Checks jobfile for syntax errors. 
 *
 * @param jobDetails line to be checked 
 * 
 * @return true if no syntax error detected, false otherwise. 
 */ 
bool validate_jobfile_syntax(char* jobDetails) {
    remove_newline(jobDetails); 
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
    if (sscanf(details, "%[^,]%n,%lf%n,%lf%n,%d%n,%d%n", function, 
            &functionCount, &lower, &lowerCount, &upper, 
            &upperCount, &segments, &segmentsCount, 
            &threads, &threadsCount) != 5) {
        return false; 
    }
    char threadsAfter[FUNCTION_BUFFER]; 
    sprintf(threadsAfter, "%d", threads); 
    char segmentsAfter[FUNCTION_BUFFER]; 
    sprintf(segmentsAfter, "%d", segments);  
    char** checkDetails = split_by_commas(details);  
    int i = 0; //check for each field validity
    while (checkDetails[i]) {
        if (checkDetails[i] == NULL) {
            return false; 
        }
        i++; 
    }
    if (i != 5) {
        return false; 
    }
    //lower & upper 
    if ((int)strlen(checkDetails[1]) > (lowerCount - functionCount - 1) || 
            (int)strlen(checkDetails[2]) > (upperCount - lowerCount - 1)) {
        return false; 
    } 
    //segments & threads 
    if ((int)strlen(checkDetails[3]) > (segmentsCount - lowerCount - 1) 
            || (int)strlen(checkDetails[4]) > 
            (threadsCount - segmentsCount - 1) || 
            strcmp(threadsAfter, checkDetails[4]) != 0 || 
            strcmp(segmentsAfter, checkDetails[3]) != 0) {
        return false; 
    }
    free(checkDetails);
    return true; 
}

/**
 * Checks if server response is OK or Bad Response. 
 *
 * Returns true if status being checked is 200, false otherwise. 
 *
 * @param status the status number being checked 
 */ 
bool server_response(int status) {
    if (status == 200) {
        return true; 
    }
    return false; 
}

/**
 * Constructs and sends HTTP request to server for function validation.
 *
 * @param jobDetails line of comma separated job details 
 * @param serverFd server's file descriptor 
 * @param lineNum line number 
 */ 
void request_validate_expression(char* jobDetails, int serverFd, 
        int lineNum) {
    JobFile job = { NULL, 0, 0, 0, 0}; 
    job = parse_jobdetails(jobDetails, job); 

    char validateFunction[FUNCTION_BUFFER]; 
    strcpy(validateFunction, job.function); 
    //free(job)

    //construct HTTP request 
    char request[REQUEST_BUFFER + FUNCTION_BUFFER];
    strcpy(request, "GET /validate/"); 
    strcat(request, validateFunction); 
    strcat(request, " HTTP/1.1\r\n\r\n"); 
    
    //send HTTP request to server for validation
    if (send(serverFd, request, strlen(request),0) < 0) {
        //did not send 
    }
    
    //receive server response 
    int buffer[RESPONSE_BUFFER];
    int numChars = read(serverFd, buffer, RESPONSE_BUFFER); 
    if (numChars == 0) {
        exit_program(INVALID_COMM, NULL); 
    }
    int status = 0; 
    char* statusExplanation = NULL; 
    HttpHeader** headers = NULL; 
    char* bodyStr = NULL;
    int numCharRead = 0; 
    numCharRead = parse_HTTP_response(buffer, numChars, &status, 
            &statusExplanation, &headers, &bodyStr);
    while (numCharRead == 0) {
        numChars = read(serverFd, buffer, RESPONSE_BUFFER); 
        numCharRead = parse_HTTP_response(buffer, numChars, &status, 
                &statusExplanation, &headers, &bodyStr);
    }

    if (numCharRead < 0) {
        //badly formed or incomplete response  
    } else {
        if (!(server_response(status))) {
            fprintf(stderr, "intclient: bad expression \"%s\" (line %d)\n", 
                    validateFunction, lineNum);
        } else {
            //send_request_integration
            request_integrate_function(validateFunction, job.lower, job.upper, 
                    job.segments, job.threads, serverFd);  
        }
    }
}

/**
 * Prints out the overall result to stdout in the specified format. 
 *
 * @param function expression evaluated 
 * @param lower lower bound 
 * @param upper upper bound 
 * @param result  final result of the integration calculation 
 */ 
void emit_overall_result(char* function, double lower, double upper, 
        char* result) {
    char message[MESSAGE_BUFFER + sizeof(function) + sizeof(lower) + 
            sizeof(upper) + sizeof(result)];
    double convertResult = atof(result); 
    sprintf(message, "The integral of %s from %lf to %lf is %lf", 
            function, lower, upper, convertResult); 
    printf("%s\n", message); 
}

/**
 * Constructs and sends HTTP request to server for integration computation. 
 */ 
void request_integrate_function(char* function, double lowerVal, 
        double upperVal, int segmentsVal, int threadsVal, int serverFd) { 
    
    //construct HTTP request
    char request[RESPONSE_BUFFER]; 
    strcpy(request, "GET /integrate/"); 
    sprintf(request, "GET /integrate/%lf/%lf/%d/%d/%s HTTP/1.1\r\n\r\n", 
            lowerVal, upperVal, segmentsVal, threadsVal, function); 

    //send HTTP request to server
    if (send(serverFd, request, strlen(request),0) < 0) { 
        //did not send 
    }
  
    //receive server response 
    int recBuffer[RESPONSE_BUFFER]; 
    int numRec = read(serverFd, recBuffer, RESPONSE_BUFFER);
    if (numRec == 0) {
        exit_program(INVALID_COMM, NULL); 
    }
    int status = 0; 
    char* statusExplanation = NULL; 
    HttpHeader** headers = NULL; 
    char* result = NULL; 
    int numCharRead = parse_HTTP_response(recBuffer, numRec, &status, 
            &statusExplanation, &headers, &result);

    if (numCharRead <= 0) {
        //badly formed or incomplete response 
        fprintf(stderr, "intclient: integration failed\n"); 
    } else {
        if (!(server_response(status))) {
            fprintf(stderr, "intclient: integration failed\n"); 
        } else {
        //emit results to stdout
            strtok(result, "\n");  
            emit_overall_result(function, lowerVal, upperVal, result); 
        }
    }
}

/**
 * Reads a given jobfile. Parses contents and sends to server if no syntax or 
 * job rules errors violated. 
 *
 * @param jobfile the jobfile to be read 
 *
 * @return EXIT_NORMAL if successful and once EOF is reached 
 */ 
ExitCode read_jobfile(char* jobfile, int serverFd) {
    FILE* openFile = fopen(jobfile, "r"); 
    int lineNum = 1; 
    char* line = malloc(sizeof(char) * JOBDETAILS_BUFFER); 
    if (jobfile == NULL) {
        fgets(line, JOBDETAILS_BUFFER, stdin);
    } else {
        line = read_line(openFile);
    }
    while (line != NULL) {
        //ignore comments and blank lines  
        if (line[0] != '#' && strlen(line) > 0) { 
            if (!validate_jobfile_syntax(line)) {
                fprintf(stderr, "intclient: syntax error on line %d\n", 
                        lineNum);
                lineNum++;
                if (jobfile == NULL) {
                    if (fgets(line, 100, stdin) == NULL) {
                        free(line); 
                        exit(EXIT_NORMAL); 
                    }
                } else {
                    line = read_line(openFile); 
                }
                continue; 
            }
            JobError jobError = validate_jobfile(line, lineNum); 
            if (jobError != NO_ERRORS) {
                jobfile_error(jobError, lineNum);
            } else {
                //send validate request to server
                request_validate_expression(line, serverFd, lineNum); 
            }
        }
        lineNum++; 
        if (jobfile != NULL) {
            line = read_line(openFile);
        } else {
            if (fgets(line, FUNCTION_BUFFER, stdin) == NULL) {
                free(line);
                exit(EXIT_NORMAL); 
            }   
        } 
    }
    free(line); 
    return EXIT_NORMAL; 
}

/**
 * Takes in input from STDIN; these are the job details (comma separated). 
 *
 * @param serverFd server's file descriptor 
 */ 
ExitCode read_stdin(int serverFd) {
    // retrieve user input 
    char input[JOBDETAILS_BUFFER];
    scanf("%s", input);

    //send user input to server 
    request_validate_expression(input, serverFd, 1); 

    return EXIT_NORMAL; 
}

int main(int argc, char** argv) {
    ClientArgs cmdArgs;
    cmdArgs = process_cmd_args(argc, argv); 
    
    FILE* jobfile; 
    if (cmdArgs.jobfile != NULL) {
        jobfile = fopen(cmdArgs.jobfile, "r"); 
        if (!jobfile) {
            fprintf(stderr, "intclient: unable to open \"%s\" for reading\n", 
                    cmdArgs.jobfile);
            exit(INVALID_JOBFILE); 
        }
    }
    //Connect to server given portNum
    int serverFd = connect_to_server(cmdArgs.portNum);
    int err = EXIT_NORMAL; 
    //Read and Parse jobfile (or stdin), one line at a time. 
    err = read_jobfile(cmdArgs.jobfile, serverFd);
     
    close(serverFd); 
    fclose(jobfile); 
    exit_program(err, NULL); 
}

/** 
 * Processes command line arguments.
 *
 * @param argc total number of arguments passed 
 * @param arguments passed from command line 
 */ 
ClientArgs process_cmd_args(int argc, char** argv) {
    ClientArgs param = {false, NULL, NULL}; //on user input 
    //validate cmd args 
    if (argc == 1 || argc > 4) {
        exit_program(INVALID_CMD, NULL); 
    }
    if (strcmp(argv[1], "-v") == 0 && argc == 2) {
        exit_program(INVALID_CMD, NULL); 
    } else if (strcmp(argv[1], "-v") != 0 && (argc > 3 || argc < 2)) {
        exit_program(INVALID_CMD, NULL); 
    } else {
        //parses through cmd args 
        argv++; 
        argc--; 
        if (strcmp(argv[0], "-v") == 0 && !param.v) {
            param.v = true;
            argv++; 
            argc--; 
        } 
        while (argc > 0) {
            if (param.portNum == NULL) {
                param.portNum = argv[0];
            } else if (param.jobfile == NULL) {
                param.jobfile = argv[0]; 
            }
            argc--; 
            argv++; 
        }
    }
    return param; 
}
