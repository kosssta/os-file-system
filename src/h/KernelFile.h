#pragma once

#include "windows.h"
using namespace std;

typedef unsigned long BytesCnt;
typedef unsigned long ClusterNo;
class KernelFS;
class SharedData;
class Semaphore;

class KernelFile {
	bool canWrite;
	BytesCnt cursor = 0;
	KernelFS* fs = nullptr;
	SharedData *shared = nullptr;
	friend class KernelFS;
public:
	KernelFile();
	~KernelFile(); //zatvaranje fajla
	char write(BytesCnt, char* buffer);
	BytesCnt read(BytesCnt, char* buffer);
	char seek(BytesCnt);
	BytesCnt filePos();
	char eof();
	BytesCnt getFileSize();
	char truncate();
};
