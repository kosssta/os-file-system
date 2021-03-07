#include "SharedData.h"
#include "semaphore.h"

SharedData::SharedData(char *fname, ClusterNo index)
{
	BytesCnt length = strlen(fname);
	this->fname = new char[length + 1];
	for (BytesCnt i = 0; i < length; i++)
		this->fname[i] = fname[i];
	this->fname[length] = '\0';
	this->index = index;
	numReader = numOpen = size = 0;
	db = new Semaphore(1);
	mutexR = new Semaphore(1);
}


SharedData::~SharedData()
{
	delete mutex; mutex = nullptr;
	delete fname; fname = nullptr;
}

void SharedData::reset() {
	size = numOpen = numReader = 0;
}
