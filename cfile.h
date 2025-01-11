#pragma once
#include "filesystem.h"

typedef struct CFILE {
	struct FileSystemInfo fs;
	struct FileInfo file;
	uint32_t current_pos;
} CFile;

void CFileInit(uint32_t filesystem_start_sector);
CFile* Cfopen(const char* path,const char*open_mode);
int Cfread(void* buffer, uint32_t size, uint32_t count, CFile* file);
int Cfwrite(const void* buffer, uint32_t size, uint32_t count, CFile* file);
int Cfseek(CFile* file, int offset, int origin);
int Cftell(CFile* file);
int Cfclose(CFile* file);