#include "base_io.h"

FILE* disk_fp;

int OpenVDisk(const char* path)
{
	if (disk_fp != 0)
	{
		fclose(disk_fp);
	}
	disk_fp = fopen(path, "r+b");
	if (disk_fp == 0)
	{
		return -1;
	}
	return 0;
}

int MakeVDisk(const char* path)
{
	FILE* fp = fopen(path, "r+b");
	if (fp != 0)
	{
		fclose(fp);
		return -1;
	}
	fp = fopen(path, "wb");
	if (fp == 0)
	{
		return -1;
	}
	fclose(fp);
	return 0;
}

void ReadSector(void* buffer, unsigned int sector_id, unsigned int need_num)
{
	_fseeki64(disk_fp, sector_id * 512, SEEK_SET);
	size_t size = need_num * 512;
	fread(buffer, size, 1, disk_fp);
}

void WriteSector(void* buffer, unsigned int sector_id, unsigned int num)
{
	_fseeki64(disk_fp, sector_id * 512, SEEK_SET);
	size_t size = num * 512;
	fwrite(buffer, size, 1, disk_fp);
}

void* kmalloc(unsigned int size)
{
	return malloc(size);
}

void kfree(void* ptr)
{
	free(ptr);
}

void End()
{
	if (disk_fp)
		fclose(disk_fp);
}