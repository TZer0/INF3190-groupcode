#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/time.h>
#include <unistd.h>

#define MAX_LEN 40
#define DISPLAY_PROMPT do { printf("yolo >> "); fflush(stdout); } while (0)
// stderr is *not* linebuffered, so we can avoid fflush call by:
//#define DISPLAY_PROMPT fprintf(stderr, "yolo >> ");


typedef void (*cb_func_t)(void *arg);
typedef struct timeout_entry timeout_t;

struct timeout_entry {
    struct timeval timeout;
    cb_func_t callback_func;
    void *args;
    // TODO: Linked list of timeout entries
    //timeout_t *next;
};

static int has_asked_for_help = 0;
static timeout_t *timeout;


static void free_timeout()
{
    if (timeout)
        free(timeout);
}


static void register_timeout(int msec, cb_func_t callback_func, void *args)
{
    // TODO: Add support for multiple timeouts (linked list)
    if (timeout)
        return;

    timeout_t *t = malloc(sizeof(timeout_t));
    assert(t);

    struct timeval start, period;
    gettimeofday(&start, NULL);
    period.tv_sec = msec / 1000;
    period.tv_usec = (msec % 1000) * 1000;

    timeradd(&start, &period, &t->timeout);
    
    t->callback_func = callback_func;
    t->args = args;

    timeout = t;
}


static void check_timeout()
{
    if (timeout) {
        struct timeval now;
        gettimeofday(&now, NULL);
        
        if (timercmp(&timeout->timeout, &now, <)) {
            timeout_t *t = timeout;
            timeout = NULL;
            t->callback_func(t->args);
            free(t);
        }
    }
}


static void display_hint()
{
    if (!has_asked_for_help) {
        printf("\nHint: try the command \"help\"\n");
        DISPLAY_PROMPT;
        register_timeout(3000, &display_hint, NULL);
    }
}


static void handle_keyboard()
{
    char command[MAX_LEN] = {'\0'};
    assert(fgets(command, MAX_LEN, stdin));
    command[strlen(command)-1] = '\0';

    // TODO:
    // Split into separate helper functions for different commands
        
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
        has_asked_for_help = 1;
    }

    DISPLAY_PROMPT;
}


int main(void)
{
    DISPLAY_PROMPT;

    // The cleanup function passed to atexit can't take arguments
    // Another function on_exit supports passing arguments as well
    atexit(&free_timeout);

    register_timeout(3000, &display_hint, NULL);

    fd_set readfds;
    FD_ZERO(&readfds);
    FD_SET(STDIN_FILENO, &readfds);

    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 100000;

    while (1)
    {
        fd_set readfds_copy = readfds;
        struct timeval tv_copy = tv;

        int retval = select(STDIN_FILENO+1, &readfds_copy, NULL, NULL, &tv_copy);
        assert(retval >= 0);

        check_timeout();

        // TODO: Check for retval == 0 if we have more than one FD
        if (FD_ISSET(STDIN_FILENO, &readfds_copy))
            handle_keyboard();
    }
}
