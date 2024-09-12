#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>
#include <assert.h>
#include <fcntl.h>

#define MAX_INPUT_SIZE 1024
#define MAX_ARG_COUNT 64

char* paths[64] = {"/bin"};
int path_count = 1;

// Function to parse the input into command and arguments
void parseInput(char* input, char** args, char** arrayArgs) {
    // Handle newline character
    input[strcspn(input, "\n")] = '\0';

    int i;
    for (i = 0; i < MAX_ARG_COUNT; i++) {
        args[i] = strsep(&input, " ");
        if (args[i] == NULL) break;
        
        if (strlen(args[i]) == 0) {
            i--; // Skip empty strings
        } else {
            arrayArgs[i] = args[i]; // Store in arrayArgs
        }
    }
    arrayArgs[i] = NULL; 
}

void pathDomain(char** args) {
    if (args[1] == NULL && path_count>1) {
        for (int i = 1; i < path_count; i++) {
            free(paths[i]);
        }
        path_count = 1;
    } else {
        // Add new paths to the existing list
        for (int i = 1; args[i] != NULL; i++) {
            paths[path_count++] = strdup(args[i]);
        }
    }
}

void executeCommand(char** arrayArgs) {
     if (arrayArgs[0] == NULL) return;

    // In-built path
     if (strcmp(arrayArgs[0], "path") == 0) {
        pathDomain(arrayArgs);
        //return;
    }

    // Print command
    for (int i = 0; arrayArgs[i] != NULL; i++) {
        printf("Argument %d: %s\n", i, arrayArgs[i]);
    }

    for (int i = 0; i < path_count; i++) {
            printf("Path %d: %s\n", i, paths[i]);
    }
}


int main(int MainArgc, char *MainArgv[]){

    // Input and arguments definitions
    char* input = NULL;size_t len = 0;ssize_t dataSize;
    char* args[MAX_ARG_COUNT];
    char* arrayArgs[MAX_ARG_COUNT];
    FILE* inputFile = stdin;

    // Batch Mode handler
    if (MainArgc == 2) {
        inputFile = fopen(MainArgv[1], "r"); // Read the file
        if (inputFile == NULL) {
            //handle error here
            return 1;
        }
    }

    // Loop for Interactive and Batch mode
    while (1) {
        // Print the prompt in interactive mode
        if (inputFile == stdin) {
            printf("witsshell> ");
            fflush(stdout);
        }

        // Read the user input 
        dataSize = getline(&input, &len, inputFile);

        //error handling #1 (invalid input given to getline())
        if (dataSize == -1) {
            if (feof(inputFile)) break; // Exit on EOF
            continue;
        }

        //handle input 
        parseInput(input, args, arrayArgs);

        // Exit handler
        if (strcmp(input, "exit") == 0) {
            break;
        }

        executeCommand(arrayArgs);
        
    }

    // Cleanup (evaluate)
    free(input); // Free the input buffer
    if (inputFile != stdin) {
        fclose(inputFile); // Close the batch file if in batch mode
    }

    //batch mode
    return 0;
}