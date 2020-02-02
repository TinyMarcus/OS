/*
В программу с программным каналом включить собственный обработчик сигнала. 
Использовать сигнал для изменения хода выполнения программы.
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>

#define BUFFER_SIZE 64
#define TIME 5

static int signal_is_catched = 0;
static int fd[2];
static char buffer[BUFFER_SIZE];

void catch_signal(int sig_num)
{
    signal_is_catched = 1;
}

int main()
{
	int childPIDS[2];
	int status, wpid;
    char msg[] = "Important message from child #";

    signal(SIGINT, catch_signal);

    if (pipe(fd) == -1)
    {
        exit(1);
    }

	for (int i = 0; i < 2; i++)
	{
		if ((childPIDS[i] = fork()) == -1)
		{
			perror("Can't fork.\n");
            exit(1);
		}
		if (childPIDS[i] == 0)
		{
			sleep(TIME);

            if (signal_is_catched)
            {
                msg[30] = '0' + i + 1;
                close(fd[0]);
                write(fd[1], msg, BUFFER_SIZE);
            }
            exit(0);
		}
	}

    sleep(TIME);
    close(fd[1]);
    printf("\n");

    for (int i = 0; i < 2 && signal_is_catched; i++)
    {
        read(fd[0], buffer, BUFFER_SIZE);
        printf("Message from child: %s\n", buffer);
    }

	for (int i = 0; i < 2; i++)
	{
        wpid = wait(&status);
        if (wpid == -1)
        {
			perror("waitpid");
			exit(EXIT_FAILURE);
		}
        
		if (WIFEXITED(status))
		{
			printf("\tChild exited with status = %d\n", WEXITSTATUS(status));
		}
		else if (WIFSIGNALED(status))
		{
			printf("\tChild with (signal %d) killed\n", WTERMSIG(status));
		}
		else if (WIFSTOPPED(status))
		{
			printf("\tChild with (signal %d) stopped\n", WSTOPSIG(status));
		}
		else
			printf("\tUnexpected status for Child (0x%x)\n", status);
	}

    printf("OK!\n");
	return 0;
}


