#include "fs.h"
#include "KernelFS.h"

KernelFS* FS::myImpl = new KernelFS;

FS::FS() {}

FS::~FS() {
	delete myImpl;
	myImpl = nullptr;
}

char FS::mount(Partition* partition)
{
	//ili mozda ovde wait(sem)
	return myImpl->mount(partition);
}

char FS::unmount()
{
	//wait(sem)
	return myImpl->unmount();
}

char FS::format()
{
	return myImpl->format();
}

FileCnt FS::readRootDir()
{
	return myImpl->readRootDir();
}

char FS::doesExist(char * fname)
{
	return myImpl->doesExist(fname);
}

File * FS::open(char * fname, char mode)
{
	return myImpl->open(fname, mode);
}

char FS::deleteFile(char * fname)
{
	return myImpl->deleteFile(fname);
}
