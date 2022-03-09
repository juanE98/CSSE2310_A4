#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h> 
#include <unistd.h>
#include <ctype.h>
#include <string.h>
#include <pthread.h>
#include <tinyexpr.h>
#include <limits.h>
#include "shared.h" 

/* CSSE2310 Library */ 
#include <tinyexpr.h> 
#include <csse2310a4.h> 
#include <csse2310a3.h> 

#define USAGE_ERROR 1

#define FUNCTION_BUFFER 20 
#define ADDRESS_BUFFER 50 
#define REQUEST_BUFFER 150 
#define RESPONSE_BUFFER 255 

#define INVALID_PORT 3

/** 
 * Stores information about the server. Used to pass between functions and 
 * threads. 
 */
typedef struct {
    int maxThreads;
    int currentThreads; 
    char* port; 
} Server; 

struct Compute {
    char* expression; 
    double lower; 
    double upper; 
    int segments; 
    int threads;
};
/****************************************************************************/

/**
 * Listens to given port on localhost and returns the fd of the socket to 
 * accept if successful or returns -1 if failed. 
 *
 * @param port Given port numer or service name 
 */ 
int listen_to(const char* port, Server server) {
    struct addrinfo hints; 
    struct addrinfo* ai = 0; 
    memset(&hints, 0, sizeof(struct addrinfo)); 
    hints.ai_family = AF_INET; 
    hints.ai_socktype = SOCK_STREAM; 

    int err; 
    if ((err = getaddrinfo(NULL, port, &hints, &ai))) {
        freeaddrinfo(ai);
        fprintf(stderr, "intserver: unable to open socket for listening\n"); 
        exit(INVALID_PORT); 
    }
    //create a socket and bind it to port
    int listenFd = socket(AF_INET, SOCK_STREAM, 0); 
    if (listenFd < 0) {
        fprintf(stderr, "intserver: unable to open socket for listening\n"); 
        exit(INVALID_PORT); 
    }
    //Allow port number to be reused immediately 
    setsockopt(listenFd, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int));
    if (bind(listenFd, (struct sockaddr*)ai->ai_addr, sizeof(struct sockaddr)) 
            < 0) {
        close(listenFd);  
        fprintf(stderr, "intserver: unable to open socket for listening\n"); 
        exit(INVALID_PORT); 
    }
    
    //listen and wait for incoming connections from client(s) 
    if (listen(listenFd, SOMAXCONN)) {
         
    }

    //Print out which port we got
    struct sockaddr_in ad;
    memset(&ad, 0, sizeof(struct sockaddr_in));
    socklen_t len = sizeof(struct sockaddr_in);
    if (getsockname(listenFd, (struct sockaddr*) &ad, &len)) {
        //error getting port name
    }
    fprintf(stderr, "%u\n", ntohs(ad.sin_port));
    fflush(stderr);

    return listenFd; 
}

/**
 * Validates given expression from client. Returns true if expression if valid,
 * false otherwise. 
 *
 * @param expression Function given by client  
 */ 
bool validate_expression(char* expression) {
    double x; 
    te_variable vars[] = {{"x", &x}}; 

    int error; 
    te_expr* expr = te_compile(expression, vars, 1, &error); 
    te_free(expr); 

    if (error != 0) {
        return false; 
    }
    return true; 
}

/**
 * Function to spawn computation threads 
 */ 
void spawn_computation_threads(char** function, double lower, double upper, 
        int segments, int threads){

}

/**
 * Evaluates and integrates a function by the trapezoidal method given an
 * expression and its relevant parameters. 
 *
 * @param function expression to be evaluated 
 * @param lower lower bound 
 * @param upper upper bound 
 * @param segments number of trapezoidal segments 
 */ 
double calculate_integration(char* function, double lower, double upper, 
        int segments) {
    double x; 
    double result = 0;  

    te_variable vars[] = {{"x", &x}}; 

    int err; 

    double parts = (upper - lower) / ((double)segments);
    
    te_expr* expr = te_compile(function, vars, 1, &err); 

    x = lower; 
    if (expr) {
        for (int i = 1; i < segments; i++) {
            x += parts;  
            double trapezoid = te_eval(expr); 
            result += ((double) 2 * trapezoid); 
        }
        x = lower; 
        double trapezoid = te_eval(expr); 
        result += trapezoid; 

        x = upper; 
        trapezoid = te_eval(expr); 
        result += trapezoid; 

        result *= (parts / (double) 2); 
        te_free(expr); 
        return result; 
    } else {
        //error parsing expression, return 0
        return result; 
    }
}

/**
 * Helper function to deal with server's response when integrate request is 
 * OK. 
 */ 
void integrate_response(char** finalResult, int*** status, 
        char*** responseStatus, HttpHeader***** headers, char**** body, 
        int threads) {
    strcpy(**responseStatus, "OK"); 
    ***status = 200;
    HttpHeader** cpyHeader = ***headers; 
    int i = 0; 
    bool verbose = false; 
    while (*cpyHeader != NULL) {
        if (strcmp(cpyHeader[i]->name, "X-Verbose") == 0 
                && strcmp(cpyHeader[i]->value, "yes") == 0) {
            verbose = true;
            break; 
        }
        i++; 
    }

    char* verboseOutput = malloc(sizeof(char*) * REQUEST_BUFFER);
    if (verbose) {
        //verbose result integration 

        //sprintf(verboseOutput, "thread %d:%lf->%lf:%lf", 
        //threadNum, lowerBound, upperBound, intResult); 
        strcat(***body, *finalResult); 
        strcat(***body, "\n"); 

    } else {
        strcat(***body, *finalResult); 
        strcat(***body, "\n"); 
    }
    free(verboseOutput); 
 
}

/**
 * Validates parameters given to server similar to client. Expression 
 * validation included. 
 */ 
void validate_integration_request(char*** fields, char** responseStatus, 
        int** status, HttpHeader**** headers, char*** body) {
    char* lower = malloc(sizeof(char*) * FUNCTION_BUFFER); 
    char* upper = malloc(sizeof(char*) * FUNCTION_BUFFER); 
    char* segments = malloc(sizeof(char*) * FUNCTION_BUFFER); 
    char* threads = malloc(sizeof(char*) * FUNCTION_BUFFER); 
    char* expression = malloc(sizeof(char*) * FUNCTION_BUFFER);
    strtok(**fields, "/"); 
    strcpy(lower, strtok(NULL, "/")); 
    strcpy(upper, strtok(NULL, "/")); 
    strcpy(segments, strtok(NULL, "/")); 
    strcpy(threads, strtok(NULL, "/")); 
    strcpy(expression, strtok(NULL, "/"));
    //create a comma separated string and send it to validate_jobfile()
    char* validateDetails = malloc(sizeof(char*) * ADDRESS_BUFFER); 
    create_comma_strings(&expression, &validateDetails);
    create_comma_strings(&lower, &validateDetails);
    create_comma_strings(&upper, &validateDetails);
    create_comma_strings(&segments, &validateDetails);
    create_comma_strings(&threads, &validateDetails);
    if (validate_jobfile(validateDetails, 0) != NO_ERRORS 
            || !validate_expression(expression)) { 
        strcpy(*responseStatus, "Bad Request");
        **status = 400; 
    } else {
    //no errors, proceed to integrate function.
        struct Compute compute;
        compute.expression = expression; 
        compute.lower = atof(lower);
        compute.upper = atof(upper); 
        compute.segments = atoi(segments); 
        compute.threads = atoi(threads);
    //spawn computation threads here...

        double result = calculate_integration(compute.expression, 
                compute.lower, compute.upper, compute.segments);
        char* finalResult = malloc(sizeof(char*) * FUNCTION_BUFFER); 
        sprintf(finalResult, "%lf", result);

    //send result back to client, need to construct HTTP response back 
        integrate_response(&finalResult, &status, &responseStatus, 
                &headers, &body, compute.threads);
        free(finalResult); 
    }
    free(validateDetails); 
    free(lower); 
    free(upper); 
    free(segments); 
    free(threads); 
    free(expression);   
}

/**
 * Server's reponse given an expression request from client. 
 */ 
void server_response(int* status, int fd, HttpHeader*** headers, 
        char** body, char** address, char** method) {
    char* responseStatus = malloc(sizeof(char) * REQUEST_BUFFER); 
    char* response = malloc(sizeof(char) * RESPONSE_BUFFER); 
    //expression validation 
    char* addressCpy = malloc(sizeof(char*) * strlen(*address)); 
    strcpy(addressCpy, *address); 
    char* expression = *address + 10;  
    char* methodCompare = malloc(sizeof(char*) * strlen(*method));  
    char methodGet[5] = "GET";
    char addressField[ADDRESS_BUFFER]; 

    strcpy(addressField, strtok(addressCpy, "/")); 
    strcpy(methodCompare, *method);

    if (validate_expression(expression) && 
            strcmp(methodCompare, methodGet) == 0 
            && (strcmp(addressField, "validate") == 0)) {
        strcpy(responseStatus, "OK");
        *status = 200;

    } else if (strcmp(methodCompare, methodGet) == 0 
            && (strcmp(addressField, "integrate") == 0)) {
        //server response to integration request 
        validate_integration_request(&address, &responseStatus, 
                &status, &headers, &body);

    } else {
        strcpy(responseStatus, "Bad Request");
        *status = 400; 
    }
    response = construct_HTTP_response(*status, responseStatus, 
            *headers, *body);
    if (send(fd, response, strlen(response),0) < 0) {
        //erorr in sending response to client
    }
    free(response); 
    free(responseStatus);
    free(addressCpy); 
    free(methodCompare); 
}

/**
 * Client thread spawned for each incomming connection received. 
 */ 
void* client_handler_thread(void* arg) {
    int fd = *(int*) arg; 
    free(arg);  
    int status = 0; 
    int buffer[REQUEST_BUFFER + REQUEST_BUFFER]; 
    //receive client request 
    int numChars = 0;
    char* method = NULL; 
    char* address = NULL; 
    HttpHeader** headers = NULL; 
    char* body = NULL; 
    int readBytes = read(fd, buffer, REQUEST_BUFFER); 
    while (readBytes > 0) {
        numChars = parse_HTTP_request(buffer, readBytes, &method, 
                &address, &headers, &body);  
        if (numChars < 0) {
            //badly formed request
            close(fd); 
            pthread_exit(NULL);
        } else if (numChars > 0) {
            //respond back to client
            server_response(&status, fd, &headers, &body, &address, &method);
        } else {
            //incomplete request, keep reading 
            readBytes = read(fd, buffer, REQUEST_BUFFER); 
        }
        readBytes = read(fd, buffer, REQUEST_BUFFER); 
    } 

    if (readBytes == 0) { //read EOF is reached 
        close(fd);
    } 

    pthread_exit(NULL); 
    //return NULL; 
}

/**
 * 
 */ 
void process_connections(int fdServer, Server server) {
    int fd; 
    struct sockaddr_in fromAddr; 
    socklen_t fromAddrSize; 

    //Continuously accept connections and process data (integration) 
    while (1) {
        fromAddrSize = sizeof(struct sockaddr_in);
        fd = accept(fdServer, (struct sockaddr*)&fromAddr, &fromAddrSize); 
        if (fd < 0) { //Error accepting connection 
        }
        //start a thread to deal with client 
        int* fD = malloc(sizeof(int));
        *fD = fd; 
        pthread_t clientThread;
        pthread_create(&clientThread, NULL, client_handler_thread, fD); 
        pthread_detach(clientThread); 
    }
}

int main(int argc, char** argv) {
    Server server; 
    int fdServer;
    int maxThreads = INT_MAX;  
    char* port;
    if (argc == 3) {
        maxThreads = atoi(argv[2]); 
    } 
    if (argc > 3 || argc <= 1) {
        fprintf(stderr, "Usage: intserver portnum [maxthreads]\n"); 
        exit(USAGE_ERROR); 
    } 
    port = argv[1]; 
    int portNum = atoi(argv[1]);

    if (portNum < 0 || portNum > 65535 || maxThreads <= 0 
            || !is_number(port)) {
        fprintf(stderr, "Usage: intserver portnum [maxthreads]\n");
        exit(USAGE_ERROR); 
    } 

    server.maxThreads = maxThreads; 
    server.currentThreads = 0; 
    server.port = port; 

    //listen 
    fdServer = listen_to(port, server);

    //process connections 
    process_connections(fdServer, server); 
    return 0; 
}
