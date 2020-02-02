#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define WRITERS 3
#define READERS 5

#define ACTIVE_WRITER 0
#define WAITING_WRITERS 1
#define ACTIVE_READERS 2
#define WAITING_READERS 3

const int PERMS = S_IRWXU | S_IRWXG | S_IRWXO;

struct sembuf start_read[5] = {{WAITING_READERS, 1, SEM_UNDO}, 
                               {WAITING_WRITERS, 0, SEM_UNDO}, 
                               {ACTIVE_WRITER, 0, SEM_UNDO}, 
                               {WAITING_READERS, -1, SEM_UNDO}, 
                               {ACTIVE_READERS, 1, SEM_UNDO}};

struct sembuf stop_read[1] = {{ACTIVE_READERS, -1, SEM_UNDO}};

struct sembuf start_write[5] = {{WAITING_WRITERS, 1, SEM_UNDO}, 
                                {ACTIVE_WRITER, 0, SEM_UNDO}, 
                                {ACTIVE_READERS, 0, SEM_UNDO}, 
                                {WAITING_WRITERS, -1, SEM_UNDO}, 
                                {ACTIVE_WRITER, 1, SEM_UNDO}};

struct sembuf stop_write[1] = {{ACTIVE_WRITER, -1, SEM_UNDO}};

int *shared;

void reader(const int semid, const int index)
{
    srand(time(NULL));
    sleep(rand() % 10);
    int sem_op;
    if ((sem_op = semop(semid, start_read, 5)) == -1)
    {
        perror("Failed to semop\n");
        exit(1);
    }
    printf("Reader %d read %d\n", index - WRITERS, *shared);
    if ((sem_op = semop(semid, stop_read, 1)) == -1)
    {
        perror("Failed to semop\n");
        exit(1);
    }
}

void writer(const int semid, const int index)
{
    srand(time(NULL));
    sleep(rand() % 10);
    int sem_op;
    if ((sem_op = semop(semid, start_write, 5)) == -1)
    {
        perror("Failed to semop\n");
        exit(1);
    }
    (*shared)++;
    printf("Writer %d write %d\n", index, *shared);
    if ((sem_op = semop(semid, stop_write, 1)) == -1)
    {
        perror("Failed to semop\n");
        exit(1);
    }
}

int main()
{
    int semid, shmid;
    int ctl_ar, ctl_wr, ctl_aw, ctl_ww;
    int cpid;
    int status;
    pid_t pid[WRITERS + READERS];

    if ((semid = semget(IPC_PRIVATE, 4, IPC_CREAT | PERMS)) == -1)
    {
        perror("Failed to semget\n");
	    exit(1);
    }
    ctl_ar = semctl(semid, ACTIVE_READERS, SETVAL, 0);
    ctl_wr = semctl(semid, WAITING_READERS, SETVAL, 0); 
    ctl_aw = semctl(semid, ACTIVE_WRITER, SETVAL, 0); 
    ctl_ww = semctl(semid, WAITING_WRITERS, SETVAL, 0); 
    if (ctl_ar == -1 || ctl_wr == -1 || ctl_aw == -1 || ctl_ww == -1)
    {
        perror( "Failed to semctl\n" );
        exit(1);
    }
    if ((shmid = shmget(IPC_PRIVATE, sizeof(int), IPC_CREAT | PERMS)) == -1) 
    {
        perror("Failed to shmget\n");
        exit(1);
    }
    shared = shmat(shmid, 0, 0);
    if (*shared == -1)
    {
        perror("Failed to shmat\n");
        exit(1);
    }
    (*shared) = 0;
    for (int i = 0; i < WRITERS; i++)
    {
        pid[i] = fork();
        if (pid[i] == -1)
        {
            perror("Can't fork\n");
            exit(1);
        }
        if (pid[i] == 0)
        {
            writer(semid, i);
            return 0;
        }
    }
    for (int i = WRITERS; i < WRITERS + READERS; i++)
    {
        pid[i] = fork();
        if (pid[i] == -1)
        {
            perror("Can't fork\n");
            exit(1);
        }
        if (pid[i] == 0)
        {
            reader(semid, i);
            return 0;
        }
    }
    while (wait(&status) != -1){}
    if (shmdt(shared) == -1) 
    {
        perror( "Failed to smdt\n" );
        exit(1);
    }
    return 0;
}


