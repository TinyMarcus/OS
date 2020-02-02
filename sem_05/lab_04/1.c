/*
Написать программу, запускающую новый процесс системным вызовом fork(). 
В предке вывести собственный идентификатор ( функция getpid()), идентификатор 
группы ( функция getpgrp())  и идентификатор потомка. В процессе-потомке вывести 
собственный идентификатор, идентификатор предка ( функция getppid()) и идентификатор 
группы. Убедиться, что при завершении процесса-предка потомок, который продолжает 
выполняться, получает идентификатор предка (PPID), равный 1 или 
идентификатор процесса-посредника.
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main()
{
	int check_child = 0;
	int childPIDS[2];

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

	if (childPIDS[0] > 0 && childPIDS[1] > 0)
	{
		printf("Parent: PID = %d, group = %d, children = %d, %d\n", getpid(), getpgrp(), childPIDS[0], childPIDS[1]);
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
	}

	return 0;
}

