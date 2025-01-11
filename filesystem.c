#include "filesystem.h"

#define CEIL(a,b) ((a)+(b)-1)/(b)

struct _FileNode
{
	uint32_t FileInfoCluster;
	uint32_t NextIndex;
	uint32_t Index[0];
};

struct _FileInfo
{
	uint32_t FileSize;
	uint32_t CreateTime;
	uint32_t LastModifyTime;
	uint32_t LastAccessTime;
	uint32_t ParentNode;
	uint32_t FileType;
	char FileName[0];
};

static int ReadCluster(uint32_t cluster, int cluster_size, int sector_in_cluster, char* buffer);
static int WriteCluster(uint32_t cluster, int cluster_size, int sector_in_cluster, const void* buffer);
static uint32_t AllocatCluster(struct FileSystemInfo* fs);
static int FreeCluster(struct FileSystemInfo* fs, uint32_t cluster);
static void SignClusterUsed(struct FileSystemInfo* fs, uint32_t cluster);
static void Strcpy(char* dst, const char* src);
static void CopyBuffer(char* dst, char* src);

static void CopyBuffer(char* dst, char* src)
{
	for (int i = 0; i < 512/sizeof(uint32_t); ++i)
	{
		*(dst++) = *(src++);
	}
}

static void Strcpy(char* dst, const char* src)
{
	while (*dst++ = *src++);
}

static int Strcmp(const char* a, const char* b)
{
	while (*a && *b)
	{
		if (*a != *b) return -1;
		++a, ++b;
	}
	if (*a != *b) return -1;
	return 0;
}

inline static void ConvertSectorToCluster(uint32_t sector, int cluster_size, uint32_t* cluster, uint32_t* sector_in_cluster)
{
	*cluster = sector / cluster_size;
	*sector_in_cluster = sector % cluster_size;
}

inline static uint32_t ConvertClusterToSector(uint32_t cluster, int cluster_size, uint32_t sector_in_cluster)
{
	return cluster * cluster_size + sector_in_cluster;
}

static int ReadCluster(uint32_t cluster, int cluster_size, int sector_in_cluster, char* buffer)
{
	//������ʵ����
	int real_sector = cluster * cluster_size + sector_in_cluster;
	//��ȡ����
	ReadSector(buffer, real_sector, 1);
	return 0;
}

static int WriteCluster(uint32_t cluster, int cluster_size, int sector_in_cluster, const void* buffer)
{
	//������ʵ����
	uint32_t real_sector = cluster * cluster_size + sector_in_cluster;
	//д������
	WriteSector(buffer, real_sector, 1);
	return 0;
}

static uint32_t AllocatCluster(struct FileSystemInfo* fs)
{
	//��ȡλͼ��ʼ�ض�Ӧ�Ĵ�
	uint32_t start_cluster = fs->StartSector / fs->ClusterSize + 1;
	char buffer[512];
	uint32_t cluster = 0;
	//��ȡλͼ��Ѱ�ҿմ�
		while (1)
		{
			for (uint32_t sector_in_cluster = 0; sector_in_cluster < fs->ClusterSize; ++sector_in_cluster)
			{
				ReadCluster(start_cluster + cluster, fs->ClusterSize, sector_in_cluster, buffer);
				for (int i = 0; i < 512; ++i)
				{
					if (buffer[i] != 0xff)
					{
						//�ҵ��մ�
						for (int bit_in_byte = 0; bit_in_byte < 8; ++bit_in_byte)
						{
							if ((buffer[i] & (1 << bit_in_byte)) == 0)
							{
								//�ҵ���λ
								buffer[i] |= (1 << bit_in_byte);
								WriteCluster(start_cluster + cluster, fs->ClusterSize, sector_in_cluster, buffer);
								return cluster * fs->ClusterSize * 512 * 8 + sector_in_cluster * 512 * 8 + i * 8 + bit_in_byte;
							}
						}
					}
				}
			}
			cluster += 1;
		}
	
	return 0;
}

static int FreeCluster(struct FileSystemInfo* fs, uint32_t cluster)
{
	//��ȡλͼ��ʼ�ض�Ӧ�Ĵ�
	uint32_t start_cluster = fs->StartSector / fs->ClusterSize + 1;
	//����Ҫ�ͷŵĴ�λ��λͼ�ĵڼ�λ
	uint32_t bit = cluster;
	//���㱾λ���ڴغʹ�������ƫ�ƺ�������ƫ��
	uint32_t m_cluster = start_cluster + bit / (fs->ClusterSize * 512 * 8);
	uint32_t sector_offset = bit % (fs->ClusterSize * 512 * 8) / (512 * 8);
	uint32_t bit_offset = bit % (512 * 8);

	char buffer[512];
	ReadCluster(m_cluster, fs->ClusterSize, sector_offset, buffer);
	buffer[bit_offset / 8] &= ~(1 << (bit_offset % 8));
	WriteCluster(m_cluster, fs->ClusterSize, sector_offset, buffer);
}

static void SignClusterUsed(struct FileSystemInfo* fs, uint32_t cluster)
{
	//��ȡλͼ��ʼ�ض�Ӧ�Ĵ�
	uint32_t start_cluster = fs->StartSector / fs->ClusterSize + 1;
	//����Ҫ�ͷŵĴ�λ��λͼ�ĵڼ�λ
	uint32_t bit = cluster;
	//���㱾λ���ڴغʹ�������ƫ�ƺ�������ƫ��
	uint32_t m_cluster = start_cluster + bit / (fs->ClusterSize * 512 * 8);
	uint32_t sector_offset = bit % (fs->ClusterSize * 512 * 8) / (512 * 8);
	uint32_t bit_offset = bit % (512 * 8);

	char buffer[512];
	ReadCluster(m_cluster, fs->ClusterSize, sector_offset, buffer);
	buffer[bit_offset / 8] |= (1 << (bit_offset % 8));
	WriteCluster(m_cluster, fs->ClusterSize, sector_offset, buffer);
}

int OpenFileSystem(uint32_t start_sector, struct FileSystemInfo* fs)
{
	//�ļ�ϵͳ��ʼ����������뵽8����
	if (start_sector % 8 != 0)
		return -1;
	char buffer[512];
	//��ȡ�ļ�ϵͳ��ʼ��
	ReadSector(buffer, start_sector, 1);
	struct FileSystemInfo* info = (struct FileSystemInfo*)buffer;
	//����ļ�ϵͳ��ʶ
	if (info->Signature[0]=='R'&& info->Signature[1]=='B' && info->Signature[2]=='S' && info->Signature[3]=='F')
	{
		//�ļ�ϵͳ��ʶ��ȷ
		*fs = *info;
		return 0;
	}
	return -1;//�ļ�ϵͳ��ʶ����
}

int CreateFileSystem(uint32_t start_sector, uint32_t cluster_count, struct FileSystemInfo* fs)
{
	struct FileSystemInfo f;
	if (start_sector%CLUSTER_SIZE!=0)
	{
		return -1;
	}
	if (cluster_count < 3)
	{
		return -1;
	}
	uint32_t start_cluster = start_sector / CLUSTER_SIZE;
	//��ʼ���ļ�ϵͳ��Ϣ
	f.Signature[0] = 'R';
	f.Signature[1] = 'B';
	f.Signature[2] = 'S';
	f.Signature[3] = 'F';
	f.ClusterSize = CLUSTER_SIZE;
	f.ClusterCount = cluster_count;
	f.StartSector = start_sector;
	
	//����λͼ��ռ����
	uint32_t bitmap_cluster_count = (cluster_count + (CLUSTER_SIZE * 512 * 8) - 1) / (CLUSTER_SIZE * 512 * 8);
	//��ʼ��λͼ
	char buffer[512] = { 0 };
	for (uint32_t i =0;i< bitmap_cluster_count; ++i)
	{
		for (int j = 0; j < CLUSTER_SIZE; ++j)
		{
			WriteCluster(start_cluster + i + 1, CLUSTER_SIZE, j, buffer);
		}
	}
	//���λͼʹ�õĴغ��ļ�ϵͳ��Ϣ��ռ��
	for (uint32_t i = 0; i < bitmap_cluster_count + 1; ++i)
	{
		SignClusterUsed(&f, start_cluster + i);
	}
	///������Ŀ¼
	//����һ������Ϊ��Ŀ¼��
	uint32_t root_cluster = AllocatCluster(&f);
	//����ش洢��Ŀ¼��Ϣ
	uint32_t root_info_cluster = AllocatCluster(&f);

	//��ʼ����Ŀ¼
	struct _FileNode* root = (struct _FileNode*)buffer;
	root->FileInfoCluster = root_info_cluster;
	root->NextIndex = 0;
	WriteCluster(root_cluster, CLUSTER_SIZE, 0, buffer);
	f.RootDirStartCluster = root_cluster;
	//��ʼ����Ŀ¼��Ϣ
	struct _FileInfo *info = (struct _FileNode*)buffer;
	info->CreateTime = 0;
	Strcpy(info->FileName , "ROOT");
	info->FileSize = 0;
	info->LastAccessTime = 0;
	info->LastModifyTime = 0;
	info->ParentNode = start_cluster;
	info->FileType = FILE_TYPE_DIR;
	WriteCluster(root_info_cluster, CLUSTER_SIZE, 0, buffer);
	//д���ļ�ϵͳ��Ϣ
	*(struct FileSystemInfo*)buffer = f;
	WriteCluster(start_cluster, CLUSTER_SIZE, 0, buffer);
	*fs = f;
	return 0;
}

int GetRootDir(struct FileSystemInfo* fs_info, struct FileInfo* file)
{
	char data[512];
	ReadCluster(fs_info->RootDirStartCluster, fs_info->ClusterSize, 0, data);
	file->StartCluster = fs_info->RootDirStartCluster;
	file->Type = FILE_TYPE_DIR;
	//��ȡ�ļ���Ϣ��
	ReadCluster(((struct _FileNode*)data)->FileInfoCluster, fs_info->ClusterSize, 0, data);
	//��ȡ�ļ���Ϣ
	ReadCluster(((struct _FileNode*)data)->FileInfoCluster, fs_info->ClusterSize, 0, data);
	struct _FileInfo* info = (struct _FileInfo*)data;
	file->CreateTime = info->CreateTime;
	file->FileSize = info->FileSize;
	file->LastAccessTime = info->LastAccessTime;
	file->LastModifyTime = info->LastModifyTime;
	return 0;
}

int GetFileFromDir(struct FileSystemInfo* fs_info, struct FileInfo* dir_info, const char* file_name,struct FileInfo* file)
{
	char* buffer = (char*)kmalloc(fs_info->ClusterSize * 512);
	struct _FileNode* info = (struct _FileInfo*)buffer;
	char data[512];
	uint32_t* end_ptr = (uint32_t*)(buffer + fs_info->ClusterSize * 512);
	//��ȡĿ¼��Ϣ
	ReadCluster(dir_info->StartCluster, fs_info->ClusterSize, 0, buffer);
	ReadCluster(((struct _FileNode*)buffer)->FileInfoCluster, fs_info->ClusterSize, 0, buffer);
	//���Ŀ���Ƿ���Ŀ¼
	if (dir_info->Type != FILE_TYPE_DIR)
	{
		kfree(buffer);
		return -1;//Ŀ�겻��Ŀ¼
	}
	//����Ŀ¼
	uint32_t s_node_num = ((struct _FileInfo*)buffer)->FileSize;//Ŀ¼���ļ���
	uint32_t now_cluster = dir_info->StartCluster;
	uint32_t readed = 0;
	while (1)
	{
		//��ȡ��ǰ��
		for (int j = 0; j < fs_info->ClusterSize; ++j)
		{
			ReadCluster(now_cluster, fs_info->ClusterSize, j, buffer + j * 512);
		}
		uint32_t* p0 = info->Index;
		while (p0 < end_ptr)
		{
			if (readed >= s_node_num)
			{
				kfree(buffer);
				return -1;//δ�ҵ�Ŀ���ļ�
			}
			//��ȡ�ļ���Ϣ
			ReadCluster(*p0, fs_info->ClusterSize, 0, data);
			ReadCluster(((struct _FileNode*)data)->FileInfoCluster, fs_info->ClusterSize, 0, data);
			struct _FileInfo* f_info = (struct _FileInfo*)data;
			if (Strcmp(f_info->FileName, file_name) == 0)
			{
				//�ҵ�Ŀ���ļ�
				file->CreateTime = f_info->CreateTime;
				file->FileSize = f_info->FileSize;
				file->LastAccessTime = f_info->LastAccessTime;
				file->LastModifyTime = f_info->LastModifyTime;
				file->StartCluster = *p0;
				file->Type = f_info->FileType;
				kfree(buffer);
				return 0;
			}
			++p0;
			++readed;
		}
		now_cluster = info->NextIndex;
	}
}

int CreateFile(struct FileSystemInfo* fs_info,struct FileInfo* dir_info, int file_type, const char* file_name)
{
	//���Ŀ���Ƿ���Ŀ¼
	if (dir_info->Type != FILE_TYPE_DIR) return -1;//Ŀ�겻��Ŀ¼
	//����Ƿ���ͬ���ļ�
	struct FileInfo file;
	if (0 == GetFileFromDir(fs_info, dir_info, file_name, &file)) return -2;//����ͬ���ļ�
	//Ϊ�µ��ļ�����ڵ�غ���Ϣ��
	uint32_t node_cluster = AllocatCluster(fs_info), info_cluster = AllocatCluster(fs_info);
	if (node_cluster == 0 || info_cluster == 0) return -2;//�����ʧ�ܣ��������޿��пռ�
	//���µĴ��д洢�ļ�������Ϣ
	char data[512];
	struct _FileNode* fnode = (struct _FileNode*)data;
	fnode->FileInfoCluster = info_cluster;
	fnode->NextIndex = 0;
	WriteCluster(node_cluster, fs_info->ClusterSize, 0, data);
	struct _FileInfo* finfo = (struct _FileInfo*)data;
	finfo->CreateTime = 0;
	Strcpy(finfo->FileName, file_name);
	finfo->FileSize = 0;
	finfo->FileType = file_type;
	finfo->LastAccessTime = 0;
	finfo->LastModifyTime = 0;
	finfo->ParentNode = dir_info->StartCluster;
	WriteCluster(info_cluster, fs_info->ClusterSize, 0, data);
	//���ô���Ϣ�洢���丸�ڵ㣨�ļ��У���
	char* buffer = (char*)kmalloc(fs_info->ClusterSize * 512);
	struct _FileNode* info = (struct _FileInfo*)buffer;
	uint32_t* end_ptr = (uint32_t*)(buffer + fs_info->ClusterSize * 512);
	ReadCluster(dir_info->StartCluster, fs_info->ClusterSize, 0, buffer);
	uint32_t file_info_cluster = ((struct _FileNode*)buffer)->FileInfoCluster;
	ReadCluster(file_info_cluster, fs_info->ClusterSize, 0, data);
	uint32_t s_no_num = ((struct _FileInfo*)data)->FileSize;
	//���㵱ǰ�ļ���ʹ�õ��ڼ�������
	uint32_t now_used_cluster=0;
	if (s_no_num)
		now_used_cluster = CEIL(s_no_num, end_ptr - info->Index) - 1;
	//�������������ļ�����Ҫ�Ĵ���
	uint32_t need_cluster = CEIL((s_no_num + 1) , (end_ptr - info->Index))-1;
	//���������������Ҫ�����µĴأ�����ֱ��д��
	int is_need_new_cluster = need_cluster - now_used_cluster;
	//��λ����ǰ�ļ��е����һ����
	uint32_t now_cluster = dir_info->StartCluster;
	unsigned int item_in_cluster_index;//���ļ��ڴ��е�����
	for (uint32_t i = 0; i < now_used_cluster; ++i)
	{
		now_cluster = ((struct _FileNode*)buffer)->NextIndex;
		ReadCluster(now_cluster, fs_info->ClusterSize, 0, buffer);
	}
	if (is_need_new_cluster)
	{
		info->NextIndex = AllocatCluster(fs_info);
		WriteCluster(now_cluster, fs_info->ClusterSize, 0, buffer);
		now_cluster = info->NextIndex;
		info->NextIndex = 0;
		item_in_cluster_index = 0;
	}
	else
	{
		for (uint32_t i = 1; i < fs_info->ClusterSize; ++i)
		{
			ReadCluster(now_cluster, fs_info->ClusterSize, i, buffer + i * 512);
		}
		item_in_cluster_index = s_no_num % (end_ptr - info->Index);
	}
	//�����ļ���Ϣд���ļ���
	info->Index[item_in_cluster_index] = node_cluster;
	for (int i = 0; i < fs_info->ClusterSize; ++i)
	{
		WriteCluster(now_cluster, fs_info->ClusterSize, i, buffer + i * 512);
	}
	kfree(buffer);
	//�����ļ�����Ϣ
	((struct _FileInfo*)data)->FileSize += 1;
	WriteCluster(file_info_cluster, fs_info->ClusterSize, 0, data);
	return 0;
}

int RemoveFile(struct FileSystemInfo* fs_info,struct FileInfo* file)
{
	//��ȡĿ���ļ���Ϣ
	char* data = kmalloc(fs_info->ClusterSize*512), other[512];
	ReadCluster(file->StartCluster, fs_info->ClusterSize, 0, data);
	ReadCluster(((struct _FileNode*)data)->FileInfoCluster, fs_info->ClusterSize, 0, data);
	//���Ŀ���ļ��Ƿ����ļ���
	if (((struct _FileInfo*)data)->FileType == FILE_TYPE_DIR)
	{
		//Ŀ�����ļ���
		//����ļ����Ƿ�Ϊ��
		if (((struct _FileInfo*)data)->FileSize > 0)
		{
			kfree(data);
			return -1;//�ļ��в�Ϊ��
		}
	}
	//��ȡ���ڵ���Ϣ
	uint32_t parent_node_cluster = ((struct _FileInfo*)data)->ParentNode;
	for (int i = 0; i < fs_info->ClusterSize; ++i)
		ReadCluster(parent_node_cluster, fs_info->ClusterSize, i, data + i * 512);
	uint32_t info_node = ((struct _FileNode*)data)->FileInfoCluster;
	ReadCluster(info_node, fs_info->ClusterSize, 0, other);
	uint32_t file_size = ((struct _FileInfo*)other)->FileSize;
	if (file_size == 0)
	{
		kfree(data);
		return -1;//�ļ�Ϊ�գ��ڲ�����

	}

	//�Ӹ��ڵ���ɾ��Ŀ���ļ�
	//����Ŀ���ļ��ڸ��ڵ��е�λ��,�����һ���ļ���Ϣ�ƶ���Ŀ���ļ�λ�ã�����Ƿ���Ҫ�ͷŴء�
	//����Ҫ�ͷŴأ����޸ĵ����ڶ����ļ���Ϣ��NextIndexΪ0
	uint32_t* end_ptr = (uint32_t*)(data + fs_info->ClusterSize * 512);
	struct _FileNode* info = (struct _FileInfo*)data;
	uint32_t now_cluster = parent_node_cluster;
	uint32_t now_used_cluster = file_size / (end_ptr - info->Index);
	uint32_t last_item_in_cluster_index = (file_size - 1) % (end_ptr - info->Index);
	uint32_t cluster_indexs_len = end_ptr - info->Index;
	uint32_t target_cluster = 0;
	uint32_t target_item_in_cluster_index = 0;
	//���²��Ҳ���������һ��ѭ���Ĵ�
	uint32_t second_to_last_cluster = 0;//���Ǽ�¼�����ڶ����ر�ţ��Է���Ҫ�ͷ����һ����
	for (uint32_t i = 0; i < now_used_cluster; ++i)
	{
		//�ڲ������һ���ļ���Ϣ��ͬʱ������Ŀ���ļ���λ��
		for (int j = 0; j < cluster_indexs_len; ++j)
		{
			if (info->Index[j] == file->StartCluster)
			{
				//��¼Ŀ��λ���Ա��������
				target_cluster = now_cluster;
				target_item_in_cluster_index = j;
			}
		}
		if (i == now_used_cluster - 1)
			second_to_last_cluster = now_cluster;//��ʱ���ڵ����ڶ�����
		now_cluster = ((struct _FileNode*)data)->NextIndex;
		for (int j = 0; j < fs_info->ClusterSize; ++j)
			ReadCluster(now_cluster, fs_info->ClusterSize, j, data + j * 512);
	}
	if (target_cluster == 0)
	{
		//�����һ�����в���Ŀ���ļ�
		for (int j = 0; j < cluster_indexs_len; ++j)
		{
			if (info->Index[j] == file->StartCluster)
			{
				//��¼Ŀ��λ���Ա��������
				target_cluster = now_cluster;
				target_item_in_cluster_index = j;
			}
		}
	}
	if (target_cluster == 0)
	{
		kfree(data);
		return -2;//δ�ҵ�Ŀ���ļ�
	}
	if (now_used_cluster == 0)
	{
		//ֻ��һ����
		if (target_item_in_cluster_index != last_item_in_cluster_index)
		{
			info->Index[target_item_in_cluster_index] = info->Index[last_item_in_cluster_index];
			for (int i = 0; i < fs_info->ClusterSize; ++i)
				WriteCluster(target_cluster, fs_info->ClusterSize, i, data + i * 512);
		}
	}
	else
	{
		//�ж����
		if (target_cluster != now_cluster)
		{
			//Ŀ���ļ��������һ����
			int t = info->Index[last_item_in_cluster_index];
			//��ȡĿ�����ڴ�
			for (int i = 0; i < fs_info->ClusterSize; ++i)
				ReadCluster(target_cluster, fs_info->ClusterSize, i, data + i * 512);
			info->Index[target_item_in_cluster_index] = t;
			//д��Ŀ�����ڴ�
			for (int i = 0; i < fs_info->ClusterSize; ++i)
				WriteCluster(target_cluster, fs_info->ClusterSize, i, data + i * 512);
			//�ж����һ�����Ƿ���Ҫ�ͷ�
			if (last_item_in_cluster_index == 0)
			{
				//��Ҫ�ͷ����һ����
				FreeCluster(fs_info, now_cluster);
				//�޸ĵ����ڶ����ص�NextIndexΪ0
				ReadCluster(second_to_last_cluster, fs_info->ClusterSize, 0, data);
				((struct _FileNode*)data)->NextIndex = 0;
				WriteCluster(second_to_last_cluster, fs_info->ClusterSize, 0, data);
			}
		}
		else
		{
			//Ŀ���ļ������һ����
			if (target_item_in_cluster_index == last_item_in_cluster_index)
			{
				//�����һ���ļ���Ϣ
				if (target_item_in_cluster_index == 0)
				{
					//�ͷŸô�
					FreeCluster(fs_info, now_cluster);
					//�޸ĵ����ڶ����ص�NextIndexΪ0
					ReadCluster(second_to_last_cluster, fs_info->ClusterSize, 0, data);
					((struct _FileNode*)data)->NextIndex = 0;
					WriteCluster(second_to_last_cluster, fs_info->ClusterSize, 0, data);
				}
				//���������κ��޸�
			}
			else
			{
				//�����صĲ�ͬλ�ã�һ������Ҫ�ͷ����һ����
				info->Index[target_item_in_cluster_index] = info->Index[last_item_in_cluster_index];
				for (int i = 0; i < fs_info->ClusterSize; ++i)
					WriteCluster(target_cluster, fs_info->ClusterSize, i, data + i * 512);
			}
		}
	}
	//�����ļ�����Ϣ�ڵ�
	((struct _FileInfo*)other)->FileSize -= 1;
	WriteCluster(info_node, fs_info->ClusterSize, 0, other);
	//�ͷ��ļ�����
	now_cluster = file->StartCluster;
	ReadCluster(now_cluster, fs_info->ClusterSize, 0, data);
	FreeCluster(fs_info, info->FileInfoCluster);
	while (1)
	{
		FreeCluster(fs_info, now_cluster);
		now_cluster = info->NextIndex;
		if (now_cluster == 0) break;
		else ReadCluster(now_cluster, fs_info->ClusterSize, 0, data);
	}
	kfree(data);
	return 0;
}
//������ָ���ļ��ж�ȡ����Ϊbuffer_size�����ݵ�buffer�У���ȡƫ��Ϊoffset
int ReadFile(struct FileSystemInfo* fs_info,struct FileInfo* file_info, char* buffer, uint32_t buffer_size, uint32_t offset)
{
	char data[512];
	//��ȡ�ļ���Ϣ
	ReadCluster(file_info->StartCluster, fs_info->ClusterSize, 0, data);
	ReadCluster(((struct _FileNode*)data)->FileInfoCluster, fs_info->ClusterSize, 0, data);
	//����ļ�����
	if (((struct _FileInfo*)data)->FileType != FILE_TYPE_FILE) return -1;//Ŀ�겻���ļ�
	//����ȡƫ���Ƿ񳬳��ļ���С
	if (offset >= ((struct _FileInfo*)data)->FileSize) return -2;//��ȡƫ�Ƴ����ļ���С
	//�����ȡ�ĳ���
	uint32_t read_size = buffer_size;
	if (offset + buffer_size > ((struct _FileInfo*)data)->FileSize)
	{
		read_size = ((struct _FileInfo*)data)->FileSize - offset;
	}
	//������ʼλ��λ�ڵڼ���
	uint32_t start_cluster = offset / (fs_info->ClusterSize * 512-sizeof(struct _FileNode));
	uint32_t start_byte = offset % (fs_info->ClusterSize * 512 - sizeof(struct _FileNode));
	//��ȡ�ļ���ʼ��
	int now_cluster = file_info->StartCluster;
	ReadCluster(now_cluster, fs_info->ClusterSize, 0, data);
	//��λ��Ҫ��ȡ�Ĵ�
	for (int i = 0; i < start_cluster; ++i)
	{
		now_cluster = ((struct _FileNode*)data)->NextIndex;
		ReadCluster(now_cluster, fs_info->ClusterSize, 0, data);
	}
	//��ȡ����
	int readed = 0;
	//�����ڴ����ڴ�ŵ�����
	char* cluster_buffer = (char*)kmalloc(fs_info->ClusterSize * 512);
	while (readed<read_size)
	{
		//��ȡ��
		for (int i=0;i<4;++i)
			ReadCluster(now_cluster, fs_info->ClusterSize, i, cluster_buffer + i * 512);
		while (start_byte < fs_info->ClusterSize * 512-sizeof(struct _FileNode) && readed < read_size)
		{
			buffer[readed++] = cluster_buffer[sizeof(struct _FileNode)+start_byte++];
		}
		start_byte = 0;
		now_cluster = ((struct _FileNode*)cluster_buffer)->NextIndex;
	}
	kfree(cluster_buffer);
	return readed;
}

int WriteFile(struct FileSystemInfo* fs_info, struct FileInfo* file_info, const char* buffer, uint32_t buffer_size, uint32_t offset)
{
	char info[512];
	char* data = (char*)kmalloc(fs_info->ClusterSize * 512);
	uint32_t file_size = 0;
	//��ȡ�ļ���Ϣ
	ReadCluster(file_info->StartCluster, fs_info->ClusterSize, 0, data);
	uint32_t info_cluster = ((struct _FileNode*)data)->FileInfoCluster;
	ReadCluster(info_cluster, fs_info->ClusterSize, 0, info);
	//����ļ�����
	if (((struct _FileInfo*)info)->FileType != FILE_TYPE_FILE)
	{
		kfree(data);
		return -1;//Ŀ�겻���ļ�
	}
	file_size = ((struct _FileInfo*)info)->FileSize;
	//��λ��Ҫд��Ĵأ��������ļ���С����Ҫ�����µĴ�
	uint32_t start_cluster = offset / (fs_info->ClusterSize * 512-sizeof(struct _FileNode));
	uint32_t start_byte = offset % (fs_info->ClusterSize * 512-sizeof(struct _FileNode));
	//��ȡ�ļ���ʼ��
	int now_cluster = file_info->StartCluster;
	ReadCluster(now_cluster, fs_info->ClusterSize, 0, data);
	//��λ��Ҫд��Ĵ�
	int is_new_cluster = 0;
	for (int i = 0; i < start_cluster; ++i)
	{
		if (((struct _FileNode*)data)->NextIndex == 0)
		{
			((struct _FileNode*)data)->NextIndex = AllocatCluster(fs_info);
			WriteCluster(fs_info, fs_info->ClusterSize, 0, data);
			is_new_cluster = 1;
		}
		now_cluster = ((struct _FileNode*)data)->NextIndex;
		ReadCluster(now_cluster, fs_info->ClusterSize, 0, data);
		if (is_new_cluster)
		{
			((struct _FileNode*)data)->NextIndex = 0;
			WriteCluster(now_cluster, fs_info->ClusterSize, 0, data);
			is_new_cluster = 0;
		}
	}
	uint32_t writed = 0;
	//��ȡʣ������
	for (int i = 1; i < fs_info->ClusterSize; ++i)
	{
		ReadCluster(now_cluster, fs_info->ClusterSize, i, data + i * 512);
	}
	//��ʼѭ��д������
	is_new_cluster = 0;
	while (writed < buffer_size)
	{
		//д������
		while (start_byte < fs_info->ClusterSize * 512 - sizeof(struct _FileNode) && writed < buffer_size)
		{
			data[sizeof(struct _FileNode) + start_byte++] = buffer[writed++];
		}
		start_byte = 0;
		if (is_new_cluster)
		{
			((struct _FileNode*)data)->NextIndex = 0;
			for (int i = 1; i < fs_info->ClusterSize; ++i)
			{
				WriteCluster(now_cluster, fs_info->ClusterSize, i, data + i * 512);
			}
		}
		else
		{
			//д���
			for (int i = 0; i < 4; ++i)
			{
				WriteCluster(now_cluster, fs_info->ClusterSize, i, data + i * 512);
			}
		}
		//��д��������˳�
		if (writed == buffer_size)
		{
			WriteCluster(now_cluster, fs_info->ClusterSize, 0, data);
			break;
		}
		//�����һ���Ƿ����,������������µĴ�
		if (((struct _FileNode*)data)->NextIndex == 0)
		{
			//�����µĴ�
			((struct _FileNode*)data)->NextIndex = AllocatCluster(fs_info);
			WriteCluster(now_cluster, fs_info->ClusterSize, 0, data);
			is_new_cluster = 1;
		}
		else is_new_cluster = 0;
		now_cluster = ((struct _FileNode*)data)->NextIndex;
		if (is_new_cluster==0)
		{
			//��ȡ��һ��
			for (int i = 0; i < fs_info->ClusterSize; ++i)
			{
				ReadCluster(now_cluster, fs_info->ClusterSize, i, data + i * 512);
			}
		}
	}
	//�����ļ���Ϣ
	if (file_size < offset + buffer_size)
	{
		((struct _FileInfo*)info)->FileSize = offset + buffer_size;
		WriteCluster(info_cluster, fs_info->ClusterSize, 0, info);
	}
	kfree(data);
	return 0;
}

int GetDirFileNumber(struct FileSystemInfo* fs_info, struct FileInfo* dir_info)
{
	char data[512];
	ReadCluster(dir_info->StartCluster, fs_info->ClusterSize, 0, data);
	ReadCluster(((struct _FileNode*)data)->FileInfoCluster, fs_info->ClusterSize, 0, data);
	if (((struct _FileInfo*)data)->FileType != FILE_TYPE_DIR) return -1;//Ŀ�겻���ļ���
	return ((struct _FileInfo*)data)->FileSize;
}

int GetFileFromDirWithIndex(struct FileSystemInfo* fs_info, struct FileInfo* dir_info, int index, char* buffer, uint32_t buffer_size, struct FileInfo* file)
{
	char* data = (char*)kmalloc(fs_info->ClusterSize * 512);
	uint32_t max_len = (fs_info->ClusterSize * 512 - sizeof(struct _FileNode)) / sizeof(uint32_t);
	ReadCluster(dir_info->StartCluster, fs_info->ClusterSize, 0, data);
	ReadCluster(((struct _FileNode*)data)->FileInfoCluster, fs_info->ClusterSize, 0, data+512);
	if (((struct _FileInfo*)(data + 512))->FileType != FILE_TYPE_DIR)
	{
		kfree(data);
		return -1;//Ŀ�겻���ļ���
	}
	if (index >= ((struct _FileInfo*)(data + 512))->FileSize)
	{
		kfree(data);
		return -2;//Out of bound
	}
	//����λ�ڵڼ���
	uint32_t target_cluster = index / max_len;
	uint32_t target_index_in_cluster = index % max_len;
	
	uint32_t now_cluster = dir_info->StartCluster;
	for (int i = 0; i < target_cluster; ++i)
	{
		now_cluster = ((struct _FileNode*)(data))->NextIndex;
		ReadCluster(now_cluster, fs_info->ClusterSize, 0, data);
	}
	//��Ŀ��ض�ȡ����
	for (int i = 1; i < fs_info->ClusterSize; ++i)
		ReadCluster(now_cluster, fs_info->ClusterSize, i, data + i * 512);
	target_cluster = ((struct _FileNode*)data)->Index[target_index_in_cluster];
	//��ȡĿ���ļ�ͷ
	file->StartCluster = target_cluster;
	ReadCluster(target_cluster, fs_info->ClusterSize, 0, data);
	ReadCluster(((struct _FileNode*)data)->FileInfoCluster, fs_info->ClusterSize, 0, data);
	file->CreateTime = ((struct _FileInfo*)data)->CreateTime;
	file->FileSize = ((struct _FileInfo*)data)->FileSize;
	file->LastAccessTime = ((struct _FileInfo*)data)->LastAccessTime;
	file->LastModifyTime = ((struct _FileInfo*)data)->LastModifyTime;
	file->Type = ((struct _FileInfo*)data)->FileType;
	int i;
	for (i = 0; i < buffer_size && ((struct _FileInfo*)data)->FileName[i]; ++i)
	{
		buffer[i] = ((struct _FileInfo*)data)->FileName[i];
	}
	if (i < buffer_size) buffer[i] = 0;
	else buffer[buffer_size - 1] = 0;
	kfree(data);
	return 0;
}

void GetFileName(struct FileSystemInfo* fs_info, struct FileInfo* file, char* buffer, uint32_t buffer_size)
{
	char data[512];
	ReadCluster(file->StartCluster, fs_info->ClusterSize, 0, data);
	ReadCluster(((struct _FileNode*)data)->FileInfoCluster, fs_info->ClusterSize, 0, data);
	int i;
	for (i = 0; i < buffer_size && ((struct _FileInfo*)data)->FileName[i]; ++i)
	{
		buffer[i] = ((struct _FileInfo*)data)->FileName[i];
	}
	if (i < buffer_size) buffer[i] = 0;
	else buffer[buffer_size - 1] = 0;
}

int GetFileByPath(struct FileSystemInfo* fs_info, const char* path, struct FileInfo* file_info)
{
	char data[512];
	char name[512];
	char* p = path;
	if (*p != '/') return -1;//·��û����/��ʼ
	p += 1;
	//��ȡ�ļ�ϵͳ��Ŀ¼
	struct FileInfo fp;
	GetRootDir(fs_info, &fp);
	int i = 0;
	while (1)
	{
		if (i >= 511) return -2;
		if (*p != '/'&&*p!=0) name[i++] = *(p++);
		else
		{
			name[i] = 0;
			i = 0;
			
			if (GetFileFromDir(fs_info, &fp, name, &fp))
			{
				return -2;//·�����ļ�������
			}
			if (*p == 0) break;
			++p;
		}
	}
	*file_info = fp;
	return 0;
}
