/*
Написать программу, в которой предок и потомок обмениваются сообщением 
через программный канал.
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

int main()
{
	int check_child = 0;
	int childPIDS[2];
	int status, wpid;
    int fd[2];

    char msg[2][64] = {"Hello from Child #1", "Hello from Child #2"};
    char read_msg[64];

    if (pipe(fd) == -1)
    {
        exit(1);
    }

	for (int i = 0; i < 2 && !check_child; i++)
	{
		if ((childPIDS[i] = fork()) == -1)
		{
			perror("Can't fork.\n");
            exit(1);
		}
		if (childPIDS[i] == 0)
		{
			check_child = 1;
		}
	}

    for (int i = 0; i < 2; i++)
    {
        if (childPIDS[i] == 0)
        {
            write(fd[1], msg[i], 64);
            if (i == 0)
            {
                exit(0);
            }
            else if (i == 1)
            {
                exit(0);
            }
        }
    }

    printf("Parent %d gets messages from children:\n", getpid());

    for (int i = 0; i < 2; i++)
    {
        read(fd[0], read_msg, 64);
        printf("%s\n", read_msg);
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

	return 0;
}


