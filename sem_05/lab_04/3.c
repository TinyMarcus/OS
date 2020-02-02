/*
Написать программу, в которой процесс-потомок вызывает системный вызов exec(), 
а процесс-предок ждет завершения процесса-потомка.
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

	if (childPIDS[0] == 0)
	{
		printf("\nChild #1: childPID = %d, parentPID = %d, group = %d\n", getpid(), getppid(), getpgrp());
		if (execlp("ls", "ls", 0) == -1)
        {
            perror("Child couldn't exec.");
            exit(1);
        }
	}

	if (childPIDS[1] == 0)
	{
		printf("Child #2: childPID = %d, parentPID = %d, group = %d\n", getpid(), getppid(), getpgrp());
		if (execlp("ps", "ps", "-al", 0) == -1)
        {
            perror("Child couldn't exec.");
            exit(1);
        }
	}

	printf("Parent: PID = %d, group = %d, children = %d, %d\n", getpid(), getpgrp(), childPIDS[0], childPIDS[1]);
	
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


