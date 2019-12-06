#define _GNU_SOURCE /* For sigset_t. */

#include <sys/prctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define FILENAME_MAX_LENGTH 64
#define PATHNAME_MAX_LENGTH 1024

#define ATOMIC_BLOCK_SIZE 1024

#define DATA_PATH "/home/serge/data/"

/* Error codes. */
enum {
        WRONG_USAGE = -2,
};

static int bit;

void
child_sigusr1(int nsig); /* Receive signal that parent received previous bit. */
void
parent_sigusr1(int nsig); /* Receive bit '0'*/
void
parent_sigusr2(int nsig); /* Receive bit '1' */
void
sigchld(int nsig); /* Receive signal that child finished. */
void
sighup(int nsig); /* Receive signal from OS that parent dead. */

int
main(int argc, char *argv[])
{
        char file_name[FILENAME_MAX_LENGTH];
        pid_t ppid;
        pid_t chpid;
        sigset_t mask;
        sigset_t oldmask;

        /* Get file name. */
        if (argc < 2) {
                fprintf(stderr, "Wrong usage.\n");
                fprintf(stderr, "Use: %s <file name>.\n", argv[0]);
                exit(WRONG_USAGE);
        } else {
                strcpy(file_name, argv[1]);
        }

        /* Block some signals. */
        sigemptyset(&mask);
        sigaddset(&mask, SIGUSR1);
        sigaddset(&mask, SIGUSR2);
        sigaddset(&mask, SIGCHLD);
        sigprocmask(SIG_BLOCK, &mask, &oldmask);

        /* Get parent pid. */
        ppid = getpid();

        /* Create child. */
        chpid = fork(); /* Child pid. */
        if (chpid < 0) {
                perror("fork");
                exit(errno);
        }

        /* CHILD. */
        if (chpid == 0) {
                // const char name[] = "Child";
                int file_fd_read;
                char file_path[PATHNAME_MAX_LENGTH];
                char *buf;
                int read_size;

                // printf("Hello World! I am the %s (%d).\n", name, getpid());

                if (prctl(PR_SET_PDEATHSIG, SIGHUP) < 0) {
                        perror("prctl");
                        exit(errno);
                }

                /* Open file for reading. */
                sprintf(file_path, "%s%s", DATA_PATH, file_name);
                if ((file_fd_read = open(file_path, O_RDONLY | O_NONBLOCK)) < 0) {
                        perror("open");
                        exit(errno);
                }

                /* Prepare signals. */
                signal(SIGUSR1, &child_sigusr1);
                signal(SIGHUP, &sighup);

                /* Read file. */
                if ((buf = malloc(ATOMIC_BLOCK_SIZE)) == NULL) {
                        perror("malloc");
                        exit(errno);
                }
                while((read_size = read(file_fd_read, buf, ATOMIC_BLOCK_SIZE)) != 0) {
                        if (read_size < 0) {
                                perror("read");
                                exit(errno);
                        }
                        for (int i = 0; i < read_size; ++i) {
                                for (int j = 0; j < 8; ++j) {
                                        if ((buf[i] & (1 << (7 - j))) == 0)
                                                kill(ppid, SIGUSR1);
                                        else
                                                kill(ppid, SIGUSR2);
                                        sigsuspend(&oldmask);
                                }
                        }
                }

                free(buf);
                if (close(file_fd_read)) {
                        perror("close");
                        exit(errno);
                }
        }
        /* PARENT */
        else { /* chpid > 0 */
                // const char name[] = "Parent";
                char c;

                // printf("Hello World! I am the %s (%d).\n", name, getpid());

                /* Prepare signals. */
                signal(SIGUSR1, &parent_sigusr1);
                signal(SIGUSR2, &parent_sigusr2);
                signal(SIGCHLD, &sigchld);

                /* Write file. */
                while(true) {
                        c = '\0';
                        for (int i = 0; i < 8; ++i) {
                                sigsuspend(&oldmask);
                                c |= bit << (7 - i);
                                kill(chpid, SIGUSR1);
                        }
                        write(1, &c, 1);
                }
        }

        exit(EXIT_FAILURE);
}

/* Receive signal that parent received previous bit. */
void
child_sigusr1(int nsig)
{
        // printf("Received signal from parent.\n");
}

/* Receive bit '0'*/
void
parent_sigusr1(int nsig)
{
        // printf("Received bit '0'.\n");
        bit = 0;
}

/* Receive bit '1' */
void
parent_sigusr2(int nsig)
{
        // printf("Received bit '1'.\n");
        bit = 1;
}

/* Receive signal that child finished. */
void
sigchld(int nsig)
{
        // printf("Received signal from child.\n");
        exit(EXIT_SUCCESS);
}

/* Receive signal from OS that parent dead. */
void
sighup(int nsig)
{
        fprintf(stderr, "Parent dead.\n");
        exit(EXIT_FAILURE);
}
