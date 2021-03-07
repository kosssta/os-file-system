#pragma once

#include "part.h"

#define readCluster(num, buffer) { if (partition->readCluster(num, buffer) == 0) return 0; }
#define writeCluster(num, buffer) { if (partition->writeCluster(num, buffer) == 0) return 0; }

typedef char Data[ClusterSize];
typedef int EntryNo;
class Semaphore;

class CacheDir {
	Partition * partition;
	EntryNo n;
	ClusterNo *tag;
	Data *data;
	bool *v;
	bool *d;
	Semaphore *mutex;
	char readFromDisk(EntryNo entry, ClusterNo dataCluster);
	char writeToDisk(EntryNo entry);
public:
	CacheDir(Partition * partition, EntryNo n = 16);
	~CacheDir();
	void read(ClusterNo dataCluster, char buffer[], int offset, size_t size);
	void write(ClusterNo dataCluster, char buffer[], int offset, size_t size);
	void flush();
};