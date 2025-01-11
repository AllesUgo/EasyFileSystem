#pragma once
#include <stdlib.h>
#include <stdio.h>


void ReadSector(void* buffer, unsigned int sector_id, unsigned int need_num);
int OpenVDisk(const char* path);
int MakeVDisk(const char* path);

void WriteSector(void* buffer, unsigned int sector_id, unsigned int num);

void* kmalloc(unsigned int size);

void kfree(void* ptr);

void End();
