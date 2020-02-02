/*
Написать программу по схеме первого задания, но в процессе-предке выполнить
системный вызов wait(). Убедиться, что в этом случае идентификатор процесса 
потомка на 1 больше идентификатора процесса-предка.
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
		}
		if (childPIDS[i] == 0)
		{
			check_child = 1;
		}
	}

	if (childPIDS[0] == 0)
	{
		sleep(1);
		printf("\nChild #1: childPID = %d, parentPID = %d, group = %d\n", getpid(), getppid(), getpgrp());
		exit(0);
	}

	if (childPIDS[1] == 0)
	{
		sleep(2);
		printf("Child #2: childPID = %d, parentPID = %d, group = %d\n", getpid(), getppid(), getpgrp());
		exit(1);
	}

	printf("Parent: PID = %d, group = %d, children = %d, %d\n", getpid(), getpgrp(), childPIDS[0], childPIDS[1]);
	
	for (int i = 1; i <= 2; i++)
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


