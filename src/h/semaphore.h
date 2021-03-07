#pragma once

#include <windows.h>
using namespace std;

class Semaphore {
	HANDLE sem = nullptr;
public:
	Semaphore(int permits, int maxpermits);
	Semaphore(int permits = 1);
	DWORD wait();
	DWORD signal();
	~Semaphore();
};

class Mutex {
	Semaphore *sem;
public:
	Mutex(Semaphore *s) : sem(s) { if (sem) sem->wait(); }
	~Mutex() { if (sem) sem->signal(); }
};
