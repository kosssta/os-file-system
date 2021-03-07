#include "Cache.h"
#include "Semaphore.h"
#include <iostream>
using namespace std;

char CacheDir::writeToDisk(EntryNo entry)
{
	if (d[entry]) {
		writeCluster(tag[entry], data[entry]);
		d[entry] = false;
	}
	return 1;
}

char CacheDir::readFromDisk(EntryNo entry, ClusterNo dataCluster)
{
	readCluster(dataCluster, data[entry]);
	v[entry] = true;
	d[entry] = false;
	tag[entry] = dataCluster;
	return 1;
}

CacheDir::CacheDir(Partition * partition, EntryNo n)
{
	this->partition = partition;
	n = n > 0 ? n : 2;
	this->n = n;
	tag = new ClusterNo[n];
	data = new Data[n];
	for (EntryNo i = 0; i < n; i++)
		for (ClusterNo j = 0; j < ClusterSize; j++)
			data[i][j] = 0;
	v = new bool[n];
	d = new bool[n];
	for (int i = 0; i < n; i++)
		v[i] = d[i] = false;
	mutex = new Semaphore[n];
}

CacheDir::~CacheDir()
{
	flush();
	delete[] d;
	delete[] v;
	delete[] data;
	delete[] tag;
	delete[] mutex;
}

void CacheDir::read(ClusterNo dataCluster, char buffer[], int offset, size_t size)
{
	if (dataCluster >= 1l << 16) {
		cout << "A";
		system("pause");
		return;
	}
	EntryNo entry = dataCluster & 0xf;
	Mutex dummy(&mutex[entry]);
	if (!v[entry] || tag[entry] != dataCluster) {
		writeToDisk(entry);
		readFromDisk(entry, dataCluster);
	}
	for (size_t j = 0; j < size; j++)
		buffer[j] = data[entry][j + offset];
}

void CacheDir::write(ClusterNo dataCluster, char buffer[], int offset, size_t size)
{
	if (dataCluster >= 1l << 16) {
		cout << "A";
		system("pause");
		return;
	}
	EntryNo entry = dataCluster & 0xf;
	Mutex dummy(&mutex[entry]);

	if (!v[entry] || tag[entry] != dataCluster) {
		writeToDisk(entry);
		readFromDisk(entry, dataCluster);
	}
	d[entry] = true;
	for (size_t j = 0; j < size; j++)
		data[entry][j + offset] = buffer[j];
}

void CacheDir::flush()
{
	for (int i = 0; i < n; i++) {
		Mutex dummy(&mutex[i]);
		writeToDisk(i);
		v[i] = false;
	}
}
