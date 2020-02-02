#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/wait.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <signal.h>

struct sembuf prod_beg[2] = { {1, -1, SEM_UNDO}, {2, -1, SEM_UNDO} };
struct sembuf prod_end[2] =  { {0, 1, SEM_UNDO}, {2, 1, SEM_UNDO} };
struct sembuf cons_beg[2] = { {0, -1, SEM_UNDO}, {2, -1, SEM_UNDO} };
struct sembuf cons_end[2] =  { {1, 1, SEM_UNDO}, {2, 1, SEM_UNDO} };

const int PERM = S_IRWXU | S_IRWXG | S_IRWXO;

const int prod_cnt = 3, cons_cnt = 3;
int all_cnt = prod_cnt + cons_cnt;

int shmid, semid;
int values = 20;

int producer(int *current_prod_value, int *shared_prod_pos, pid_t *pid, int value)
{
	while (1)
	{
		sleep(1);

		int semop_prod = semop(semid, prod_beg, 2);
		
		if (semop_prod == -1)
		{
			perror("Error with operation on semaphors!");
			exit(1);
		}

		if (*current_prod_value >= values)
		{
			for (int i = 0; i < prod_cnt; i++)
			{
				if (pid[i] != 0)
				{
					kill(pid[i], SIGTERM);
				}
				else
				{
					semop_prod = semop(semid, prod_end, 2);
					
					if (semop_prod == -1)
					{
						perror("Error with operation on semaphors!");
						exit(1);
					}

					return 0;
				}
			}
		}
	
		*(shared_prod_pos + *current_prod_value) = *current_prod_value;
		printf("Prod #%d ====> %d\n", value, *current_prod_value);
		(*current_prod_value)++;

		semop_prod = semop(semid, prod_end, 2);
		
		if (semop_prod == -1)
		{
			perror("Error with operation on semaphors!");
			exit(1);
		}
	}

	return 0;
}

int consumer(int *current_cons_value, int *shared_cons_pos, pid_t *pid, int value)
{
	while(1)
	{
		sleep(1);

		int semop_cons = semop(semid, cons_beg, 2);
		
		if (semop_cons == -1)
		{
			perror("Error with operation on semaphors!");
			exit(1);
		}

		if (*current_cons_value >= values)
		{
			for (int i = cons_cnt; i < all_cnt; i++)
			{
				if (pid[i] != 0)
				{
					kill(pid[i], SIGTERM);
				}
				else
				{
					semop_cons = semop(semid, cons_end, 2);
					
					if (semop_cons == -1)
					{
						perror("Error with operation on semaphors!");
						exit(1);
					}

					return 0;
				}
			}
		}

		*current_cons_value = *(shared_cons_pos + *current_cons_value);
		printf("Cons #%d <==== %d\n", value - prod_cnt, *current_cons_value);
		(*current_cons_value)++;

		semop_cons = semop(semid, cons_end, 2);
		
		if (semop_cons == -1)
		{
			perror("Error with operation on semaphors!");
			exit(1);
		}
	}

	return 0;
}

int main()
{
	int *shared_memory;

	int *shared_cons_pos;
	int *shared_prod_pos;

	int *current_cons_value;
	int *current_prod_value;

	pid_t pid[prod_cnt + cons_cnt];
	int status;

    srand(time(NULL));
    
    if ((shmid = shmget(IPC_PRIVATE, (values+2) * sizeof(int), IPC_CREAT | PERM)) == -1) 
	{
		perror("shmget\n");
		exit( 1 );
	}
    

    current_prod_value = (int*)shmat(shmid, 0, 0);
    if (*current_prod_value == -1)
	{
		perror("shmat\n");
		exit( 1 );
	}
    
	current_cons_value = current_prod_value + 1;
	shared_memory = current_prod_value + 2;

	*current_prod_value = 0;
	*current_cons_value = 0;

	shared_prod_pos = shared_memory;
	shared_cons_pos = shared_memory;
    
    if ((semid = semget(IPC_PRIVATE, 3, IPC_CREAT | PERM)) == -1) 
	{
		perror("semget\n");
		exit( 1 );
	}
    
    int ctrl_sb = semctl(semid, 0, SETVAL, 0);
	int ctrl_se = semctl(semid, 1, SETVAL, prod_cnt + cons_cnt); 
	int ctrl_sf = semctl(semid, 2, SETVAL, 1);
    
    if ( ctrl_se == -1 || ctrl_sf == -1 || ctrl_sb == -1)
	{
		perror( "semctl\n" );
		exit( 1 );
	}
    
    for (int i = 0; i < prod_cnt; i++)
    {
        if ((pid[i] = fork()) == -1)
        {
            perror("fork prod\n");
            exit( 1 );
        }
        
        if (pid[i] == 0)
        {
            producer(current_prod_value, shared_prod_pos, pid, i);
            
            return 0;
        }
        else 
        {
            printf("== Producer child №%d: %d ==\n", i, pid[i]);
        }
    }
    
    for (int i = prod_cnt; i < all_cnt; i++)
    {
        if ((pid[i] = fork()) == -1)
        {
            perror("fork cons");
            exit( 1 );
        }
        if (pid[i] == 0)
        {
            consumer(current_cons_value, shared_cons_pos, pid, i);   
            return 0;
        }
        else
        {
            printf("== Consumer child №%d: %d ==\n", i - prod_cnt, pid[i]);
        }
    }
    
    for (int i = 0; i < 6; i++)
        wait(&status);
        
    if (shmdt(current_prod_value) == -1)
    {
        printf("shmdt\n");
        exit( 1 );
    }
    
    return 0;
}