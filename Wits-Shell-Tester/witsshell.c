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

void executeCommand(char** args) {
    for (int i = 0; args[i] != NULL; i++) {
        printf("Argument %d: %s\n", i, args[i]);
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