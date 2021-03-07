#pragma once

#include "windows.h"

class Semaphore;
typedef unsigned long BytesCnt;
typedef unsigned long ClusterNo;

class SharedData {
	BytesCnt size;
	ClusterNo index;
	char *fname;
	Semaphore *mutex = nullptr;
	unsigned long numOpen;
	Semaphore *db, *mutexR;
	volatile unsigned long numReader;
	friend class KernelFS;
	friend class KernelFile;
public:
	SharedData(char *fname, ClusterNo index);
	void reset();
	~SharedData();
};
