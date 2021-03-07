#include "KernelFile.h"
#include "KernelFS.h"
#include "SharedData.h"
#include "semaphore.h"
#include <iostream>
using namespace std;

KernelFile::KernelFile()
{
}

KernelFile::~KernelFile()
{
	if (shared) {
		if (canWrite) {
			shared->db->signal();
		}
		else {
			shared->mutexR->wait();
			--shared->numReader;
			if (shared->numReader == 0) {
				fs->cache->flush();
				shared->db->signal();
			}
			shared->mutexR->signal();
		}
		--shared->numOpen;
		if(--shared->numOpen == 0) fs->deleteFile(shared->fname);
	}
}

char KernelFile::write(BytesCnt size, char * buffer)
{
	if (!canWrite || !shared) return 0;
	
	CacheDir *cache = fs->cache;
	char index1[4], index2[4], data[ClusterSize];
	BytesCnt read = 0;
	ClusterNo ind2 = 0, dataSlot = 0;

	while (size > 0) {
		BytesCnt more = 0;

		ind2 = cursor / (fs->numOfSlotsPerCluster * ClusterSize);
		if (ind2 >= fs->numOfSlotsPerCluster) break;		// max size of file reached
		dataSlot = (cursor / ClusterSize) % fs->numOfSlotsPerCluster;

		cache->read(shared->index, index1, ind2 * 4, 4);
		if (cursor % (fs->numOfSlotsPerCluster * ClusterSize) == 0) {	// Allocate
			ClusterNo ind = fs->allocateNewIndex();
			if (ind == 0) break;
			fs->writeToSlot((unsigned char *)index1, 0, ind);
			cache->write(shared->index, index1, ind2 * 4, 4);
			ind2 = ind;
		}
		else ind2 = fs->readSlot((unsigned char *)index1, 0);
		cache->read(ind2, index2, dataSlot * 4, 4);
		//dataSlot = fs->readSlot((unsigned char *)index2, dataSlot);
		if (cursor % ClusterSize == 0) {	//Allocate
			//dataSlot = (cursor / ClusterSize) % fs->numOfSlotsPerCluster;
			ClusterNo ind = fs->allocateNewIndex();
			if (ind == 0) break;
			fs->writeToSlot((unsigned char *)index2, 0, ind);
			cache->write(ind2, index2, dataSlot * 4, 4);
			dataSlot = ind;
		}
		else dataSlot = fs->readSlot((unsigned char *)index2, 0);

		more = ClusterSize - cursor % ClusterSize;
		more = more < size ? more : size;
		cache->read(dataSlot, data, cursor % ClusterSize, more);

		for (size_t i = 0; i < more; i++)
			data[i] = buffer[read + i];

		cache->write(dataSlot, data, cursor % ClusterSize, more);
		shared->size = shared->size + more;
		cursor += more;
		read += more;
		size -= more;
	}
	ClusterNo offset;
	char buff[32];
	ClusterNo index = fs->findFile(shared->fname, &ind2, &offset, nullptr);
	if (index != 0) {
		cache->read(ind2, buff, offset * 32, 32);
		BytesCnt cnt = shared->size;
		for (int i = 16; i < 20; i++) {
			buff[i] = cnt & 0xff;
			cnt >>= 8;
		}
		cache->write(ind2, buff, offset * 32, 32);
	}

	return index != 0 ? 1 : 0;
}

BytesCnt KernelFile::read(BytesCnt size, char * buffer)
{
	CacheDir* cache = fs->cache;
	char index1[4], index2[4], data[ClusterSize];
	BytesCnt read = 0;
	ClusterNo ind2 = 0, dataSlot = 0;
	if (!shared) return 0;
	
	BytesCnt curSize = shared->size;
	if (cursor + size > curSize) size = curSize - cursor;
	ClusterNo index = shared->index;
	while (size > 0) {
		BytesCnt more = 0;

		ind2 = cursor / (fs->numOfSlotsPerCluster * ClusterSize);
		if (ind2 >= fs->numOfSlotsPerCluster) break;		//	max size of file reached
		cache->read(index, index1, ind2 * 4, 4);
		dataSlot = (cursor / ClusterSize) % fs->numOfSlotsPerCluster;
		ind2 = fs->readSlot((unsigned char *)index1, 0);
		if (ind2 == 0) break;
		cache->read(ind2, index2, dataSlot * 4, 4);
		dataSlot = fs->readSlot((unsigned char *)index2, 0);
		if (dataSlot == 0) break;

		more = ClusterSize - cursor % ClusterSize;
		more = more < size ? more : size;
		cache->read(dataSlot, data, cursor % ClusterSize, more);

		for(size_t i = 0; i < more; i++)
			buffer[read + i] = data[i];

		cursor += more;
		read += more;
		size -= more;
	}

	return 1;
}

char KernelFile::seek(BytesCnt pos)
{
	if (!shared || pos >= shared->size) return 0;
	cursor = pos;
	return 1;
}

BytesCnt KernelFile::filePos()
{
	return cursor;
}

char KernelFile::eof()
{
	if (!shared || cursor > shared->size) return 1;
	return cursor == shared->size ? 2 : 0;
}

BytesCnt KernelFile::getFileSize()
{
	return shared ? shared->size : 0;
}

char KernelFile::truncate()
{
	return 0;
}
