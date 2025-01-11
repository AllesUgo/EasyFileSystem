#include "base_io.h"
/*
�ļ�ϵͳ�ṹ:
�Դ�Ϊ��λ���й���
�ش�СΪ2048�ֽڣ�4��������
�غŴ�0��ʼ��ţ�0�Ŵ�Ϊ�ļ�ϵͳ������Ϣ
�ɻ�����Ϣ�õ�λͼ��ʼ�غ͸�Ŀ¼��ʼ��
λͼ��С��ClusterCount/8�õ�
*/

#define CLUSTER_SIZE 4
#define FILE_TYPE_FILE 0
#define FILE_TYPE_DIR 1

typedef unsigned int uint32_t;//����32λ�޷�������

struct FileSystemInfo
{
	char Signature[4];//�ļ�ϵͳ��ʶ��
	uint32_t ClusterSize;//based on sectors
	uint32_t StartSector;//�ļ�ϵͳ��ʼ����
	uint32_t ClusterCount;//�ļ�ϵͳ����
	uint32_t RootDirStartCluster;//��Ŀ¼��ʼ��
};//�����ļ�ϵͳ������

struct FileInfo
{
	uint32_t FileSize;//�ļ���С�������ļ��������ļ����е��ļ�����
	uint32_t CreateTime;
	uint32_t LastAccessTime;
	uint32_t LastModifyTime;
	uint32_t StartCluster;//w�ļ�����ʼ��
	uint32_t Type;//0 for file, 1 for directory
};//һ��ɳ����ļ�������


//���ļ�ϵͳ���򿪵��ļ�ϵͳ�ṹ���洢��fs�����У�start_sectorΪ�ļ�ϵͳ����ʼ������������뵽4��������
int OpenFileSystem(uint32_t start_sector, struct FileSystemInfo* fs);
//�����ļ�ϵͳ��start_sectorΪ�ļ�ϵͳ����ʼ������������뵽4��������
int CreateFileSystem(uint32_t start_sector, uint32_t cluster_count, struct FileSystemInfo* fs);
//��ȡ�ļ�ϵͳ���ļ��е�������
int GetRootDir(struct FileSystemInfo* fs_info, struct FileInfo* file);
int GetFileFromDir(struct FileSystemInfo* fs_info, struct FileInfo* dir_info, const char* file_name, struct FileInfo* file);
int CreateFile(struct FileSystemInfo* fs_info, struct FileInfo* dir_info, int file_type, const char* file_name);
int RemoveFile(struct FileSystemInfo* fs_info, struct FileInfo* file);//��Ŀ�����ļ������ļ��б���Ϊ��
int ReadFile(struct FileSystemInfo* fs_info, struct FileInfo* file_info, char* buffer, uint32_t buffer_size, uint32_t offset);
int WriteFile(struct FileSystemInfo* fs_info, struct FileInfo* file_info, const char* buffer, uint32_t buffer_size, uint32_t offset);
int GetDirFileNumber(struct FileSystemInfo* fs_info, struct FileInfo* dir_info);
int GetFileFromDirWithIndex(struct FileSystemInfo* fs_info, struct FileInfo* dir_info, int index, char* buffer, uint32_t buffer_size, struct FileInfo* file);
void GetFileName(struct FileSystemInfo* fs_info,struct FileInfo* file, char* buffer, uint32_t buffer_size);
int GetFileByPath(struct FileSystemInfo* fs_info, const char* path,struct FileInfo* file_info);//·����������/��ͷ�ľ���·���������ж����ַ�