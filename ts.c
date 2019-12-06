#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define FILENAME_MAX_LENGTH 64

/* Error codes. */
enum {
        WRONG_USAGE = -2,
};

int
main(int argc, char *argv[])
{
        char file_name[FILENAME_MAX_LENGTH];
        pid_t chpid;

        /* Get file name. */
        if (argc < 2) {
                fprintf(stderr, "Wrong usage.\n");
                fprintf(stderr, "Use: %s <file name>.\n", argv[0]);
                exit(WRONG_USAGE);
        } else {
                strcpy(file_name, argv[1]);
        }

        /* Create child. */
        chpid = fork(); /* Child pid. */
        if (chpid < 0) {
                perror("fork");
                exit(errno);
        }

        /* CHILD. */
        if (chpid == 0) {
                const char name[] = "Child";
                printf("Hello World! I am the %s (%d).\n", name, getpid());
                printf("%s\n", file_name);
        }
        /* PARENT */
        else { /* chpid > 0 */
                const char name[] = "Parent";
                printf("Hello World! I am the %s (%d).\n", name, getpid());
        }

        waitpid(chpid, NULL, 0);

        exit(EXIT_SUCCESS);
}
