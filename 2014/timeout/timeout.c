#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define MAX_LEN 40

static void handle_command(char *command)
{
    /* Check for exit, quit etc.*/
    const char *exit_commands[] = {"exit", "quit", "q"};
    int i;
    for (i = 0; i < 3; ++i)
    {
        if (!strcmp(command, exit_commands[i])) {
            printf("Goodbye :)\n");
            exit(EXIT_SUCCESS);
        }
    }

    /* Handle other commands */
    if (!strcmp(command, "help")) {
        printf("Thank you, here is some help:\n\t"
               "exit\tquit the program\n\t"
               "quit\tquit the program\n\t"
               "q\tquit the program\n");
    }
}


int main(void)
{
    char command[MAX_LEN] = {'\0'};
    while (1)
    {
        printf("yolo >> ");
        if (fgets(command, MAX_LEN, stdin)) {
            command[strlen(command)-1] = '\0'; // Remove newline from command
            handle_command(command);
        } else {
            perror("fgets");
            exit(EXIT_FAILURE);
        }
    }
}

