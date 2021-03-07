#pragma once

#include "Cache.h"
#include "semaphore.h"

class File;
class Semaphore;
class SharedData;
typedef unsigned long ClusterNo;
typedef long FileCnt;

class KernelFS {
	ClusterNo numOfSlotsPerCluster;
	CacheDir *cache = nullptr;
	ClusterNo freeBit = 0;
	ClusterNo freeCluster = 0;
	FileCnt numOfFiles = 0;
	ClusterNo numOfClusters = 0;
	ClusterNo rootDir = 0;
	ClusterNo rootDirFreeSlot = 0;
	ClusterNo rootDirNumSlots = 0;
	Semaphore* mutex = nullptr;
	Semaphore* mutexR = nullptr;
	Semaphore* semMount = new Semaphore(1);
	friend class KernelFile;

	void findNextFreeCluster(unsigned char bitVector[]);
	ClusterNo allocateNewIndex();
	void writeToSlot(unsigned char index[], int slot, ClusterNo value);
	ClusterNo readSlot(unsigned char index[], int slot);
	char updateRootDir(char *fname, ClusterNo index, SharedData *shared);
	ClusterNo findFile(char *fname, ClusterNo *index2, ClusterNo *offset, char** entry);
	char compareFileNames(char *fname1, char* fname2Name, char* fname2Ext);
	ClusterNo deleteFile(char *fname, bool updateRoot);
public:
	KernelFS();
	char mount(Partition* partition);
	char unmount();
	char format();
	FileCnt readRootDir();
	char doesExist(char* fname);
	File* open(char* fname, char mode);
	char deleteFile(char* fname);
	~KernelFS();
};
