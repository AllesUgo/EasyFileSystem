#include "base_io.h"
/*
文件系统结构:
以簇为单位进行管理
簇大小为2048字节（4个扇区）
簇号从0开始编号，0号簇为文件系统基本信息
由基本信息得到位图起始簇和根目录起始簇
位图大小由ClusterCount/8得到
*/

#define CLUSTER_SIZE 4
#define FILE_TYPE_FILE 0
#define FILE_TYPE_DIR 1

typedef unsigned int uint32_t;//定义32位无符号整数

struct FileSystemInfo
{
	char Signature[4];//文件系统标识符
	uint32_t ClusterSize;//based on sectors
	uint32_t StartSector;//文件系统起始扇区
	uint32_t ClusterCount;//文件系统簇数
	uint32_t RootDirStartCluster;//根目录起始簇
};//称作文件系统描述符

struct FileInfo
{
	uint32_t FileSize;//文件大小，对于文件夹则是文件夹中的文件个数
	uint32_t CreateTime;
	uint32_t LastAccessTime;
	uint32_t LastModifyTime;
	uint32_t StartCluster;//w文件的起始簇
	uint32_t Type;//0 for file, 1 for directory
};//一般可称作文件描述符


//打开文件系统，打开的文件系统结构将存储在fs参数中，start_sector为文件系统的起始扇区，必须对齐到4的整数倍
int OpenFileSystem(uint32_t start_sector, struct FileSystemInfo* fs);
//创建文件系统，start_sector为文件系统的起始扇区，必须对齐到4的整数倍
int CreateFileSystem(uint32_t start_sector, uint32_t cluster_count, struct FileSystemInfo* fs);
//获取文件系统跟文件夹的描述符
int GetRootDir(struct FileSystemInfo* fs_info, struct FileInfo* file);
int GetFileFromDir(struct FileSystemInfo* fs_info, struct FileInfo* dir_info, const char* file_name, struct FileInfo* file);
int CreateFile(struct FileSystemInfo* fs_info, struct FileInfo* dir_info, int file_type, const char* file_name);
int RemoveFile(struct FileSystemInfo* fs_info, struct FileInfo* file);//若目标是文件夹则文件夹必须为空
int ReadFile(struct FileSystemInfo* fs_info, struct FileInfo* file_info, char* buffer, uint32_t buffer_size, uint32_t offset);
int WriteFile(struct FileSystemInfo* fs_info, struct FileInfo* file_info, const char* buffer, uint32_t buffer_size, uint32_t offset);
int GetDirFileNumber(struct FileSystemInfo* fs_info, struct FileInfo* dir_info);
int GetFileFromDirWithIndex(struct FileSystemInfo* fs_info, struct FileInfo* dir_info, int index, char* buffer, uint32_t buffer_size, struct FileInfo* file);
void GetFileName(struct FileSystemInfo* fs_info,struct FileInfo* file, char* buffer, uint32_t buffer_size);
int GetFileByPath(struct FileSystemInfo* fs_info, const char* path,struct FileInfo* file_info);//路径必须是以/开头的绝对路径，不得有多余字符