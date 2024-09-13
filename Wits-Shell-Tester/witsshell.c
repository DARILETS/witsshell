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

char* paths[64] = {"/bin/"};
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

//come back and clean
void pathDomain(char** args) {
    if (args[1] == NULL && path_count > 1) {
        // Reset paths if no arguments are provided
        for (int i = 1; i < path_count; i++) {
            free(paths[i]);
        }
        path_count = 1;
    } else {
        // Iterate over each argument provided to the path command
        for (int i = 1; args[i] != NULL; i++) {
            bool exists = false;

            // Check if the path already exists
            for (int j = 0; j < path_count; j++) {
                if (strcmp(paths[j], args[i]) == 0) {
                    // If the path exists, replace it
                    paths[j] = strdup(args[i]);
                    exists = true;
                    break;
                }
            }

            // If the path doesn't exist, add it to the list
            if (!exists) {
                paths[path_count++] = strdup(args[i]);
            }
        }
    }
}

void cdDomain(char** args) {
    // Check if exactly one argument is passed
    //edit this if statement
    if (args[1] == NULL || args[2] != NULL) {
        char error_message[30] = "An error has occurred\n";
        write(STDERR_FILENO, error_message, strlen(error_message));
        return;
    }

    // Try to change directory
    if (chdir(args[1]) != 0) {
        // If chdir fails, print an error message
        char error_message[30] = "An error has occurred\n";
        write(STDERR_FILENO, error_message, strlen(error_message));
    }
}

void executeCommand(char** arrayArgs) {
     if (arrayArgs[0] == NULL) return;

    // In-built path
     if (strcmp(arrayArgs[0], "path") == 0) {
        pathDomain(arrayArgs);
        return;
    }

    // In-built cd
    if (strcmp(arrayArgs[0], "cd") == 0) {
        cdDomain(arrayArgs);
        return;
    }

    // handle external commands
    pid_t pid = fork();
    if (pid < 0) {
        // Error during fork
        char error_message[40] = "An error has occurred, fork error\n";
        write(STDERR_FILENO, error_message, strlen(error_message));
        return;
    }
    if (pid == 0) { 
        // execute the command using paths
        for (int i = 0; i < path_count; i++) {
            char fullPath[MAX_INPUT_SIZE];
            snprintf(fullPath, sizeof(fullPath), "%s%s", paths[i], arrayArgs[0]);

            //print fullpath
           // printf("Full path: %s\n", fullPath);

            // Check if the command is executable in the current path
            if (access(fullPath, X_OK) == 0) {
                execv(fullPath, arrayArgs); // Execute the command
                // If execv returns, it failed
                char error_message[40] = "An error has occurred, execv error\n";
                write(STDERR_FILENO, error_message, strlen(error_message));
                exit(1);
            }
         }

        // If no path worked, command not found
        char error_message[50] = "An error has occurred, no path worked\n";
        write(STDERR_FILENO, error_message, strlen(error_message));
        exit(1);

    } else { 
        // Parent process: Wait for the child process to complete
        int status;
        waitpid(pid, &status, 0);
    }
    
    // Print command
    // for (int i = 0; arrayArgs[i] != NULL; i++) {
    //     printf("Argument %d: %s\n", i, arrayArgs[i]);
    // }
    // for (int i = 0; i < path_count; i++) {
    //         printf("Path %d: %s\n", i, paths[i]);
    // }
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