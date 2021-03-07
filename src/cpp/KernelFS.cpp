#include "KernelFS.h"
#include "part.h"
#include "file.h"
#include "KernelFile.h"
#include "semaphore.h"
#include "SharedData.h"
#include <iostream>
using namespace std;

void KernelFS::findNextFreeCluster(unsigned char bitVector[])
{
	ClusterNo nextFree = freeCluster / 8;
	bool found = true;
	while ((bitVector[nextFree] & 0xff) == 0) {
		nextFree = (nextFree + 1) % numOfClusters;
		if (nextFree == freeCluster / 8) {
			found = false;
			break;
		}
	}
	if (!found) freeCluster = 0;
	else {
		int freeBit = 7;
		while (freeBit >= 0) {
			if (bitVector[nextFree] & 1 << freeBit) break;
			freeBit--;
		}
		freeCluster = freeBit == -1 ? 0 : nextFree * 8 + 7 - freeBit;
	}
}

ClusterNo KernelFS::allocateNewIndex()
{
	if (freeCluster == 0) return 0;		//no free space
	ClusterNo ret = freeCluster;

	//reset bit
	char vector[ClusterSize];
	cache->read(0, vector, 0, ClusterSize);
	if (freeCluster / 8 >= ClusterSize) return 0;
	vector[freeCluster / 8] &= ~(1 << (7 - freeCluster % 8));
	cache->write(0, vector, 0, ClusterSize);

	//update freeCluster
	findNextFreeCluster((unsigned char*)vector);

	//Initialize index with all zeros
	for (int i = 0; i < ClusterSize; i++)
		vector[i] = 0;
	cache->write(ret, vector, 0, ClusterSize);
	return ret;
}

void KernelFS::writeToSlot(unsigned char index[], int slot, ClusterNo value)
{
	for (int i = 0; i < 4; i++) {
		index[slot * 4 + i] = value & 0xff;
		value >>= 8;
	}
}

ClusterNo KernelFS::readSlot(unsigned char index[], int slot)
{
	ClusterNo ret = 0;
	for (int i = 0; i < 4; i++) {
		ret |= index[slot * 4 + i] << 8 * i;
	}
	return ret;
}

char KernelFS::updateRootDir(char *fname, ClusterNo index, SharedData *shared)
{
	char rootdir[ClusterSize]; unsigned char buffer[ClusterSize];
	cache->read(rootDir, rootdir, 0, ClusterSize);

	ClusterNo index1 = rootDirFreeSlot / (numOfSlotsPerCluster * rootDirNumSlots);
	if (rootDirFreeSlot % (numOfSlotsPerCluster * rootDirNumSlots) == 0) {			// Alociraj indeks drugog nivoa za root dir
		ClusterNo index = allocateNewIndex();
		if (index == 0) {
			return 0;		// no free space
		}
		writeToSlot((unsigned char*)rootdir, index1, index);

		cache->write(rootDir, rootdir, 0, ClusterSize);
	}

	//trazenje slobodnog mesta u root diru
	ClusterNo freeSlot = 0;
	cache->read(readSlot((unsigned char *)rootdir, index1), (char*)buffer, 0, ClusterSize);
	ClusterNo index2 = (rootDirFreeSlot / rootDirNumSlots) % numOfSlotsPerCluster, dataSlot = 0;
	if (rootDirFreeSlot % rootDirNumSlots == 0) {
		dataSlot = allocateNewIndex();
		if (dataSlot == 0) {
			return 0;
		}
		writeToSlot(buffer, index2, dataSlot);
		cache->write(readSlot((unsigned char*)rootdir, index1), (char*)buffer, 0, ClusterSize);
	}
	else dataSlot = readSlot(buffer, index2);

	cache->read(dataSlot, (char*)buffer, 0, ClusterSize);
	freeSlot = rootDirFreeSlot++ % rootDirNumSlots;

	//naziv fajla
	int pos;
	for (pos = 1; fname[pos] != '.'; pos++)
		buffer[freeSlot * 32 + pos - 1] = fname[pos];
	int dot = pos;
	while (pos <= 8) buffer[freeSlot * 32 + pos++ - 1] = ' ';

	//ekstenzija fajla
	int cnt = 0;
	for (int i = dot + 1; cnt < 3 && buffer[i] != '\0'; i++, cnt++)
		buffer[freeSlot * 32 + pos++ - 1] = fname[i];
	while (cnt++ < 3) buffer[freeSlot * 32 + pos++ - 1] = ' ';

	//ne koristi se
	buffer[freeSlot * 32 + pos++ - 1] = 0;

	//indeks prvog nivoa
	for (int i = 0; i < 4; i++) {
		buffer[freeSlot * 32 + pos++ - 1] = index & 0xff;
		index >>= 8;
	}
	//velicina fajla
	for (int i = 0; i < 4; i++)
		buffer[freeSlot * 32 + pos++ - 1] = 0;

	//slobodni biti
	unsigned long num = reinterpret_cast<unsigned long>(shared);
	unsigned long *off = reinterpret_cast<unsigned long *>((char*)buffer + freeSlot * 32 + pos - 1);
	*off = num;
	pos += 4;

	for (int i = 0; i < 8; i++)
		buffer[freeSlot * 32 + pos++ - 1] = 0;

	numOfFiles++;

	cache->write(dataSlot, (char*)buffer, 0, ClusterSize);
	return 1;
}

ClusterNo KernelFS::findFile(char * fname, ClusterNo *Index2, ClusterNo *offset, char **entry)
{
	char rootdir[ClusterSize], buffer[ClusterSize];
	unsigned char data[ClusterSize];
	cache->read(rootDir, rootdir, 0, ClusterSize);
	ClusterNo slot = 0, index2 = 0;

	index2 = readSlot((unsigned char *)rootdir, slot);
	while (index2 != 0) {
		cache->read(index2, buffer, 0, ClusterSize);
		ClusterNo dataSlot = 0, value = 0;
		while (dataSlot < numOfSlotsPerCluster) {
			value = readSlot((unsigned char *)buffer, dataSlot);
			if (value == 0) break;
			//if (value != 0) {
			cache->read(value, (char*)data, 0, ClusterSize);
			for (ClusterNo findex = 0; findex < rootDirNumSlots; findex++)
					if (compareFileNames(fname, (char*)data + findex * 32, (char*)data + findex * 32 + 8)) {
						char* offs = (char*)data + findex * 32 + 12; ClusterNo res = 0;
						for (int i = 0; i < 4; i++) {
							res |= offs[i] << 8 * i;
						}
						if (Index2 != nullptr) *Index2 = value;
						if (offset != nullptr) *offset = findex;
						if (entry != nullptr) {
							offs -= 12;
							for (int i = 0; i < 32; i++)
								(*entry)[i] = offs[i];
						}
						return res;
					}
			//}
			dataSlot++;
		}
		slot++;
		index2 = readSlot((unsigned char *)rootdir, slot);
	}
	return 0;
}

char KernelFS::compareFileNames(char * fname1, char * fname2Name, char * fname2Ext)
{
	int i; fname1++;
	for (i = 0; i < 8; i++) {
		if (fname1[i] == '.') break;
		else if (fname1[i] != fname2Name[i]) return 0;
	}
	if (i++ == 0) return 0;
	
	int j;
	for (j = 0; j < 3; i++, j++) {
		if (fname2Ext[j] == ' ') break;
		else if (fname1[i] != fname2Ext[j]) return 0;
	}
	if (j == 0) return 0;
	return 1;
}

ClusterNo KernelFS::deleteFile(char * fname, bool updateRoot)
{
	Mutex dummy(mutex);
	char* entry = new char[32];
	ClusterNo index2, offset, index;
	index = updateRoot ? findFile(fname, &index2, &offset, &entry) : findFile(fname, nullptr, nullptr, &entry);
	if (index == 0) return 0;
	
	//delete file;
	char bitVector[ClusterSize], buffer[ClusterSize];
	cache->read(0, bitVector, 0, ClusterSize);

	//azuriranje root dira
	if (updateRoot) {	//umanjivanje brojaca???
		cache->read(index2, buffer, 0, ClusterSize);
		buffer[offset * 32] = 0;
		cache->write(index2, buffer, 0, ClusterSize);
	}

	//azuriranje svih indeksa tog fajla u bit vektoru
	cache->read(index, buffer, 0, ClusterSize);
	unsigned char buffer2[ClusterSize];

	bitVector[index / 8] |= 1 << (7 - index % 8);
	for (index2 = 0; index2 < numOfSlotsPerCluster; index2++) {
		ClusterNo value = readSlot((unsigned char*)buffer, index2);
		if (value == 0 || value >= numOfSlotsPerCluster) break;
		bitVector[value / 8] |= 1 << (7 - value % 8);
		cache->read(value, (char*)buffer2, 0, ClusterSize);
		for (ClusterNo dataSlot = 0; dataSlot < numOfSlotsPerCluster; dataSlot++) {
			ClusterNo cl = readSlot(buffer2, dataSlot);
			if (cl == 0) break;
			bitVector[cl / 8] |= 1 << (7 - cl % 8);
		}
	}

	cache->write(0, bitVector, 0, ClusterSize);
	numOfFiles--;
	char *off = (char*)entry + 20;
	unsigned long *num = reinterpret_cast<unsigned long *>(off);
	SharedData *sh = reinterpret_cast<SharedData*>(*num);

	--sh->numOpen;

	delete[] entry;
	return index;
}

KernelFS::KernelFS()
{
	mutex = new Semaphore(1);
	mutexR = new Semaphore(1);
}

char KernelFS::mount(Partition * partition)
{
	if(!semMount) return 0;
	semMount->wait();
	cache = new CacheDir(partition);
	numOfClusters = partition->getNumOfClusters();
	freeCluster = 1;  freeBit = 1; //ovo treba u formatu valjda
	numOfSlotsPerCluster = ClusterSize / 4;
	rootDirNumSlots = ClusterSize / 32;
	return 1;
}

char KernelFS::unmount()
{
	if(!semMount || !semMount->signal()) return 0;
	cache = nullptr;
	return 1;
}

char KernelFS::format()
{
	Mutex dummy(mutex);
	ClusterNo clusterNum = (numOfClusters + ClusterSize - 1) / ClusterSize;
	char bitVector[ClusterSize];
	
	//Initialize bit vector
	for (ClusterNo i = 0; i < clusterNum; i++) {
		for (char &c : bitVector) c = (char)0xff;
		if (i == 0) {
			bitVector[0] = (char)0x7f;
		}
		cache->write(i, bitVector, 0, ClusterSize);
	}

	//Initialize root dir
	rootDir = allocateNewIndex();

	return rootDir == 0 ? 0 : 1;
}

FileCnt KernelFS::readRootDir()
{
	Mutex dummy(mutex);
	return numOfFiles;
	//return -1;
}

char KernelFS::doesExist(char * fname)
{
	Mutex dummy(mutex);
	return findFile(fname, nullptr, nullptr, nullptr) == 0 ? 0 : 1;
}

File * KernelFS::open(char * fname, char mode)
{
	Mutex dummy(mutex);
	ClusterNo index; File *file = new File; KernelFile* kfile = file->myImpl;
	char *entry = new char[32];
	index = findFile(fname, nullptr, nullptr, &entry);
	if (index == 0) {		//	File not existing, create a new one
		if (mode == 'r' || mode == 'a') {
			delete file;
			return nullptr;
		}
		index = allocateNewIndex();
		if (index == 0) {
			delete file;
			return nullptr;
		}
		SharedData *shared = new SharedData(fname, index);
		if (updateRootDir(fname, index, shared) == 0) {
			delete file;
			return nullptr;
		}

		kfile->shared = shared;
		kfile->canWrite = mode == 'r' ? false : true;
		kfile->cursor = mode == 'a' ? kfile->shared->size : 0;
		kfile->fs = this;
		++kfile->shared->numOpen;
	}
	else if (mode == 'w') {		// File existing already
		ClusterNo index = deleteFile(fname, false);
		kfile->canWrite = true;
		kfile->cursor = 0;
		kfile->fs = this;
		unsigned long *num = reinterpret_cast<unsigned long *>(entry + 20);
		kfile->shared = reinterpret_cast<SharedData*>(*num);
		kfile->shared->reset();
	}
	else {
		unsigned long *num = reinterpret_cast<unsigned long *>(entry + 20);
		kfile->shared = reinterpret_cast<SharedData*>(*num);
		kfile->canWrite = mode == 'r' ? false : true;
		kfile->cursor = mode == 'a' ? kfile->shared->size : 0;
		kfile->fs = this;
		++kfile->shared->numOpen;
	}
	if (mode == 'r') {
		kfile->shared->mutexR->wait();
		++kfile->shared->numReader;
		if (kfile->shared->numReader == 1) kfile->shared->db->wait();
		kfile->shared->mutexR->signal();
	}
	else {
		kfile->shared->db->wait();
	}
	delete entry;
	return file;
}

char KernelFS::deleteFile(char * fname)
{
	ClusterNo index = deleteFile(fname, false);	// if (freeCluster == 0) findFreeCluster();
	return index != 0 ? 1 : 0;
}

KernelFS::~KernelFS()
{
	delete mutex; delete mutexR; delete semMount;
	mutex = mutexR = semMount = nullptr;
}
