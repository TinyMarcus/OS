#include <stdio.h>
#include <windows.h>

const int READERS = 5;
const int WRITERS = 3;
const int iterations = 3;

int number = 0;

HANDLE mutex;
HANDLE can_read;
HANDLE can_write;

HANDLE writers[WRITERS];
HANDLE readers[READERS];

volatile LONG active_readers = 0;
volatile LONG waiting_readers = 0;
volatile LONG waiting_writers = 0;
bool writing = false;

void start_write()
{
	InterlockedIncrement(&waiting_writers);
	if (writing || active_readers > 0)
	{
		WaitForSingleObject(can_write, INFINITE);
	}
	WaitForSingleObject(mutex, INFINITE);
	InterlockedDecrement(&waiting_writers);
	writing = true;
	ReleaseMutex(mutex);
}

void stop_write()
{
	writing = false;
	if (WaitForSingleObject(can_read, 0) == WAIT_OBJECT_0)
	{
		SetEvent(can_read);
	}
	else
	{
		SetEvent(can_write);
	}
}

DWORD WINAPI writer(LPVOID)
{
	srand(GetCurrentThreadId());
	for (int i = 0; i < iterations; i++)
	{
		Sleep(rand() % 5 * 1000);
		start_write();
		printf("Writer %ld ====> %ld\n", GetCurrentThreadId(), ++number);
		stop_write();
	}

	return 0;
}

void start_read()
{
	InterlockedIncrement(&waiting_readers);
	if (writing || WaitForSingleObject(can_write, 0) == WAIT_OBJECT_0)
	{
		WaitForSingleObject(can_read, INFINITE);
	}
	InterlockedDecrement(&waiting_readers);
	InterlockedIncrement(&active_readers);
	SetEvent(can_read);
}

void stop_read()
{
	InterlockedDecrement(&active_readers);
	if (active_readers == 0)
	{
		SetEvent(can_write);
	}
}

DWORD WINAPI reader(LPVOID)
{
	srand(GetCurrentThreadId());
	while (number < WRITERS * iterations)
	{
		Sleep(rand() % 5 * 1000);
		start_read();
		printf("Reader %ld <==== %d\n", GetCurrentThreadId(), number);
		stop_read();
	}

	return 0;
}

int init_handles()
{
	mutex = CreateMutex(NULL, FALSE, NULL);
	if (mutex == NULL)
	{
		perror("Can't create a mutex");
		return 1;
	}

	can_read = CreateEvent(NULL, FALSE, TRUE, NULL);
	if (can_read == NULL)
	{
		perror("Can't create event can read");
		return 1;
	}

	can_write = CreateEvent(NULL, FALSE, TRUE, NULL);
	if (can_write == NULL)
	{
		perror("Can't create event can write");
		return 1;
	}
	return 0;
}

int create_threads()
{
	for (int i = 0; i < WRITERS; i++)
	{
		writers[i] = CreateThread(NULL, 0, writer, NULL, 0, NULL);
		if (writers[i] == NULL)
		{
			perror("Can't create a writer");
			return 1;
		}
	}

	for (int i = 0; i < READERS; i++)
	{
		readers[i] = CreateThread(NULL, 0, reader, NULL, 0, NULL);
		if (readers[i] == NULL)
		{
			perror("Can't create a reader");
			return 1;
		}
	}

	return 0;
}

int main()
{
	int rc = 0;

	if ((rc = init_handles()) != 0)
	{
		return rc;
	}

	if ((rc = create_threads()) != 0)
	{
		return rc;
	}

	WaitForMultipleObjects(WRITERS, writers, TRUE, INFINITE);
	WaitForMultipleObjects(READERS, readers, TRUE, INFINITE);

	CloseHandle(mutex);
	CloseHandle(can_read);
	CloseHandle(can_write);

	system("pause");

	return rc;
}