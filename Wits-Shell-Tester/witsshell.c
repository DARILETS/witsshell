#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>
#include <assert.h>
#include <fcntl.h>
#include <ctype.h>

#define MAX_INPUT_SIZE 1024
#define MAX_ARG_COUNT 64

char* paths[64];
int path_count = 1;

void handleRedirection(char** args, int redirect_location) {
    // Extract the filename
    char* filename = args[redirect_location + 1];
    args[redirect_location] = NULL; 
    args[redirect_location + 1] = NULL;// Null-terminate the command arguments

    // Open the file for writing (truncate if it exists) 
    int output = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (output < 0) {
        char error_message[30] = "An error has occurred\n";
        write(STDERR_FILENO, error_message, strlen(error_message));
        exit(1);
    }

    // Fork the process
    pid_t pid = fork();
    if (pid < 0) {
        char error_message[30] = "An error has occurred\n";
        write(STDERR_FILENO, error_message, strlen(error_message));
        close(output);
        exit(1);
    }

    if (pid == 0) { // Child process
        // Redirect stdout and stderr to the file
        dup2(output, STDOUT_FILENO);
        dup2(output, STDERR_FILENO);

        // Close the file descriptor
        close(output);

        for (int i = 0; i < path_count; i++) {
            char fullPath[MAX_INPUT_SIZE];
            snprintf(fullPath, sizeof(fullPath), "%s%s", paths[i], args[0]);
            if (access(fullPath, X_OK) == 0) {
                execv(fullPath, args);
                char error_message[30] = "An error has occurred\n";
                write(STDERR_FILENO, error_message, strlen(error_message));
                exit(1);
            }
         }

        // If no path worked, command not found
        char error_message[50] = "An error has occurred\n";
        write(STDERR_FILENO, error_message, strlen(error_message));
        exit(1);
    } else { // Parent process
        close(output);
        waitpid(pid, NULL, 0); // Wait for the child process to finish
    }
}

// Function to parse the input into command and arguments
void parseInput(char* input, char** args, char** arrayArgs) {
    char* redirect = strchr(input, '>');
    
    if (redirect != NULL) {
        int pos = redirect - input; // Index of ">" in the input string

        // Check if the character before ">" is a space, if not, adjust
        if (pos > 0 && input[pos - 1] != ' ') {
            // Shift all characters starting from pos-1 one place to the right
            memmove(&input[pos + 1], &input[pos], strlen(input) - pos + 1);
            input[pos] = ' '; // Insert space before ">"
            pos++; // Adjust pos due to the shift
        }

        // Check if the character after ">" is a space, if not, adjust
        if (input[pos + 1] != ' ') {
            // Shift all characters after ">" one place to the right
            memmove(&input[pos + 2], &input[pos + 1], strlen(input) - pos);
            input[pos + 1] = ' '; // Insert space after ">"
        }
    }

    int i;
    for (i = 0; i < MAX_ARG_COUNT; i++) {
        args[i] = strsep(&input, " ");
        //handle special case of ">"
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
    if (args[1] == NULL) {
        // Reset paths if no arguments are provided
        for (int i = 0; i < path_count; i++) {
            free(paths[i]);
        }
        path_count = 0;
    } else {
        for (int i = 1; args[i] != NULL; i++) {
            bool exists = false;
            char fullPath[MAX_INPUT_SIZE];

            // Check if the path already exists
            for (int j = 0; j < path_count; j++) {
                if (strcmp(paths[j], args[i]) == 0) {
                    // If the path exists, replace it
                    paths[j] = strdup(args[i]);
                    exists = true;
                    break;
                }
            }

            if (!exists) {
                if (args[i][0] == '/') {
                    // If the input is an absolute path (starts with /), use it as is
                    snprintf(fullPath, sizeof(fullPath), "%s", args[i]);
                } else {
                    // If the input is a relative path, prepend the current working directory
                    char cwd[MAX_INPUT_SIZE];
                    if (getcwd(cwd, sizeof(cwd)) != NULL) {
                        snprintf(fullPath, sizeof(fullPath), "%s/%s/", cwd, args[i]);
                    } else {
                        // Handle error in getting current directory
                        char error_message[30] = "An error has occurred\n";
                        write(STDERR_FILENO, error_message, strlen(error_message));
                        return;
                    }
                }

                // Add the constructed fullPath to the paths array
                paths[path_count++] = strdup(fullPath);
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

    //handle &
    if (strcmp(arrayArgs[0], "&") == 0) {
            return;
    }

    // In-built path
     if (strcmp(arrayArgs[0], "path") == 0) {
        pathDomain(arrayArgs);
        // Print path
    //     for (int i = 0; i < path_count; i++) {
    //         printf("Path %d: %s\n", i, paths[i]);
    // }
        return;
    }


    // In-built cd
    if (strcmp(arrayArgs[0], "cd") == 0) {
        cdDomain(arrayArgs);
        return;
    }

    //no command before redirection
    if (strcmp(arrayArgs[0], ">") == 0) {
        char error_message[30] = "An error has occurred\n";
        write(STDERR_FILENO, error_message, strlen(error_message));
        exit(0);
    }
    //handle redirection:
    int redirect_location = -1;
    for (int i = 0; arrayArgs[i] != NULL; i++) {
        if (strcmp(arrayArgs[i], ">") == 0) {
            redirect_location = i;
            break;
        }
    }


    if (redirect_location != -1) {
        // Ensure there's only one redirection operator
        if (arrayArgs[redirect_location + 1] == NULL || 
            arrayArgs[redirect_location + 2] != NULL) {
            char error_message[30] = "An error has occurred\n";
            write(STDERR_FILENO, error_message, strlen(error_message));
            return;
    }

    handleRedirection(arrayArgs, redirect_location);
    }
    else{
    // handle external commands without redirection
    pid_t pid = fork();
    if (pid < 0) {
        // Error during fork
        char error_message[30] = "An error has occurred\n";
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
                char error_message[30] = "An error has occurred\n";
                write(STDERR_FILENO, error_message, strlen(error_message));
                exit(1);
            }
         }

        // If no path worked, command not found
        char error_message[30] = "An error has occurred\n";
        write(STDERR_FILENO, error_message, strlen(error_message));
        exit(1);

    } else { 
        // Parent process: Wait for the child process to complete
        int status;
        waitpid(pid, &status, 0);
    }
    
    }
}

//edit
void runParallelCommands(char* input) {

    char* commands[MAX_ARG_COUNT];
    char* command = strtok(input, "&");

    int cmd_count = 0;
    while (command != NULL) {
        // Remove leading and trailing whitespace from the command
        while (isspace((unsigned char)*command)) command++;
        if (*command != '\0') { // Only add non-empty commands
            commands[cmd_count++] = command;
        }
        command = strtok(NULL, "&");
    }

    pid_t pids[MAX_ARG_COUNT];
    for (int i = 0; i < cmd_count; i++) {
        pids[i] = fork();
        if (pids[i] == 0) {
            char* args[MAX_ARG_COUNT];
            char* arrayArgs[MAX_ARG_COUNT];
            parseInput(commands[i], args, arrayArgs);
            executeCommand(arrayArgs);
            exit(0);
        }
    }

    for (int i = 0; i < cmd_count; i++) {
        waitpid(pids[i], NULL, 0);
    }
}


int main(int MainArgc, char *MainArgv[]){

    // Input and arguments definitions
    char* input = NULL;size_t len = 0;ssize_t dataSize;
    char* args[MAX_ARG_COUNT];
    char* arrayArgs[MAX_ARG_COUNT];
    FILE* inputFile = stdin;
    paths[0] = strdup("/bin/");

    // Batch Mode handler
    if (MainArgc == 2) {
    inputFile = fopen(MainArgv[1], "r"); // Read the file
    if (inputFile == NULL) {
        // Handle error if the file can't be opened
        char error_message[30] = "An error has occurred\n";
        write(STDERR_FILENO, error_message, strlen(error_message));
        return 1;
    }
    } else if (MainArgc > 2) {
        // Handle error if more than one file is provided
        char error_message[30] = "An error has occurred\n";
        write(STDERR_FILENO, error_message, strlen(error_message));
        return 1;
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

        // Handle newline character
        input[strcspn(input, "\n")] = '\0';
        
        // Exit handler
        if (strcmp(input, "exit") == 0) {
            parseInput(input, args, arrayArgs);

            if (args[1] != NULL) {
            char error_message[30] = "An error has occurred\n";
            write(STDERR_FILENO, error_message, strlen(error_message));
            }
            else{
            break;
            }
        }
        else if (strchr(input, '&')){
            runParallelCommands(input);
        }
        else{
            //handle input 
            parseInput(input, args, arrayArgs);
            executeCommand(arrayArgs);
        }

        
        
    }

    // Cleanup (evaluate)
    free(input); // Free the input buffer
    if (inputFile != stdin) {
        fclose(inputFile); // Close the batch file if in batch mode
    }

    //batch mode
    return 0;
}