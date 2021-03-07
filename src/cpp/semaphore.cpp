#include "semaphore.h"

Semaphore::Semaphore(int permits, int maxpermits)
{
	sem = CreateSemaphore(nullptr, permits, maxpermits, nullptr);
}

Semaphore::Semaphore(int permits)
{
	sem = CreateSemaphore(nullptr, permits, 1, nullptr);
}

DWORD Semaphore::wait()
{
	return sem ? WaitForSingleObject(sem, INFINITE) : 0;
}

DWORD Semaphore::signal()
{
	return sem ? ReleaseSemaphore(sem, 1, nullptr) : 0;
}

Semaphore::~Semaphore()
{
	if (sem) CloseHandle(sem);
}
