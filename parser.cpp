#include <string.h>

int parse_input(char *input, char *command, char *directory1, char *directory2) {
    char *delimiter = NULL;
    char *token = NULL;

    if (!input || !command || !directory1 || !directory2) {
        return 0;
    }

    input[strlen(input) - 1] = '\0';

    delimiter = " ";
    token = strtok(input, delimiter);

    if (token) {
        strcpy(command, token);
    }
    else {
        command = NULL;
    }

    token = strtok(NULL, delimiter);

    if (token) {
        strcpy(directory1, token);
    }
    else {
        directory1 = NULL;
    }

    token = strtok(NULL, delimiter);

    if (token) {
        strcpy(directory2, token);
    }
    else {
        directory2 = NULL;
    }

    return 1;
}


