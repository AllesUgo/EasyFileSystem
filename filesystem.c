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
	//计算真实扇区
	int real_sector = cluster * cluster_size + sector_in_cluster;
	//读取扇区
	ReadSector(buffer, real_sector, 1);
	return 0;
}

static int WriteCluster(uint32_t cluster, int cluster_size, int sector_in_cluster, const void* buffer)
{
	//计算真实扇区
	uint32_t real_sector = cluster * cluster_size + sector_in_cluster;
	//写入扇区
	WriteSector(buffer, real_sector, 1);
	return 0;
}

static uint32_t AllocatCluster(struct FileSystemInfo* fs)
{
	//获取位图起始簇对应的簇
	uint32_t start_cluster = fs->StartSector / fs->ClusterSize + 1;
	char buffer[512];
	uint32_t cluster = 0;
	//读取位图并寻找空簇
		while (1)
		{
			for (uint32_t sector_in_cluster = 0; sector_in_cluster < fs->ClusterSize; ++sector_in_cluster)
			{
				ReadCluster(start_cluster + cluster, fs->ClusterSize, sector_in_cluster, buffer);
				for (int i = 0; i < 512; ++i)
				{
					if (buffer[i] != 0xff)
					{
						//找到空簇
						for (int bit_in_byte = 0; bit_in_byte < 8; ++bit_in_byte)
						{
							if ((buffer[i] & (1 << bit_in_byte)) == 0)
							{
								//找到空位
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
	//获取位图起始簇对应的簇
	uint32_t start_cluster = fs->StartSector / fs->ClusterSize + 1;
	//计算要释放的簇位于位图的第几位
	uint32_t bit = cluster;
	//计算本位所在簇和簇内扇区偏移和扇区内偏移
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
	//获取位图起始簇对应的簇
	uint32_t start_cluster = fs->StartSector / fs->ClusterSize + 1;
	//计算要释放的簇位于位图的第几位
	uint32_t bit = cluster;
	//计算本位所在簇和簇内扇区偏移和扇区内偏移
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
	//文件系统起始扇区必须对齐到8扇区
	if (start_sector % 8 != 0)
		return -1;
	char buffer[512];
	//读取文件系统起始簇
	ReadSector(buffer, start_sector, 1);
	struct FileSystemInfo* info = (struct FileSystemInfo*)buffer;
	//检查文件系统标识
	if (info->Signature[0]=='R'&& info->Signature[1]=='B' && info->Signature[2]=='S' && info->Signature[3]=='F')
	{
		//文件系统标识正确
		*fs = *info;
		return 0;
	}
	return -1;//文件系统标识错误
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
	//初始化文件系统信息
	f.Signature[0] = 'R';
	f.Signature[1] = 'B';
	f.Signature[2] = 'S';
	f.Signature[3] = 'F';
	f.ClusterSize = CLUSTER_SIZE;
	f.ClusterCount = cluster_count;
	f.StartSector = start_sector;
	
	//计算位图所占簇数
	uint32_t bitmap_cluster_count = (cluster_count + (CLUSTER_SIZE * 512 * 8) - 1) / (CLUSTER_SIZE * 512 * 8);
	//初始化位图
	char buffer[512] = { 0 };
	for (uint32_t i =0;i< bitmap_cluster_count; ++i)
	{
		for (int j = 0; j < CLUSTER_SIZE; ++j)
		{
			WriteCluster(start_cluster + i + 1, CLUSTER_SIZE, j, buffer);
		}
	}
	//标记位图使用的簇和文件系统信息所占簇
	for (uint32_t i = 0; i < bitmap_cluster_count + 1; ++i)
	{
		SignClusterUsed(&f, start_cluster + i);
	}
	///创建根目录
	//分配一个簇作为根目录簇
	uint32_t root_cluster = AllocatCluster(&f);
	//分配簇存储根目录信息
	uint32_t root_info_cluster = AllocatCluster(&f);

	//初始化根目录
	struct _FileNode* root = (struct _FileNode*)buffer;
	root->FileInfoCluster = root_info_cluster;
	root->NextIndex = 0;
	WriteCluster(root_cluster, CLUSTER_SIZE, 0, buffer);
	f.RootDirStartCluster = root_cluster;
	//初始化根目录信息
	struct _FileInfo *info = (struct _FileNode*)buffer;
	info->CreateTime = 0;
	Strcpy(info->FileName , "ROOT");
	info->FileSize = 0;
	info->LastAccessTime = 0;
	info->LastModifyTime = 0;
	info->ParentNode = start_cluster;
	info->FileType = FILE_TYPE_DIR;
	WriteCluster(root_info_cluster, CLUSTER_SIZE, 0, buffer);
	//写入文件系统信息
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
	//读取文件信息簇
	ReadCluster(((struct _FileNode*)data)->FileInfoCluster, fs_info->ClusterSize, 0, data);
	//读取文件信息
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
	//读取目录信息
	ReadCluster(dir_info->StartCluster, fs_info->ClusterSize, 0, buffer);
	ReadCluster(((struct _FileNode*)buffer)->FileInfoCluster, fs_info->ClusterSize, 0, buffer);
	//检查目标是否是目录
	if (dir_info->Type != FILE_TYPE_DIR)
	{
		kfree(buffer);
		return -1;//目标不是目录
	}
	//遍历目录
	uint32_t s_node_num = ((struct _FileInfo*)buffer)->FileSize;//目录下文件数
	uint32_t now_cluster = dir_info->StartCluster;
	uint32_t readed = 0;
	while (1)
	{
		//读取当前簇
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
				return -1;//未找到目标文件
			}
			//读取文件信息
			ReadCluster(*p0, fs_info->ClusterSize, 0, data);
			ReadCluster(((struct _FileNode*)data)->FileInfoCluster, fs_info->ClusterSize, 0, data);
			struct _FileInfo* f_info = (struct _FileInfo*)data;
			if (Strcmp(f_info->FileName, file_name) == 0)
			{
				//找到目标文件
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
	//检查目标是否是目录
	if (dir_info->Type != FILE_TYPE_DIR) return -1;//目标不是目录
	//检查是否有同名文件
	struct FileInfo file;
	if (0 == GetFileFromDir(fs_info, dir_info, file_name, &file)) return -2;//已有同名文件
	//为新的文件申请节点簇和信息簇
	uint32_t node_cluster = AllocatCluster(fs_info), info_cluster = AllocatCluster(fs_info);
	if (node_cluster == 0 || info_cluster == 0) return -2;//申请簇失败，可能是无空闲空间
	//在新的簇中存储文件基本信息
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
	//将该簇信息存储到其父节点（文件夹）中
	char* buffer = (char*)kmalloc(fs_info->ClusterSize * 512);
	struct _FileNode* info = (struct _FileInfo*)buffer;
	uint32_t* end_ptr = (uint32_t*)(buffer + fs_info->ClusterSize * 512);
	ReadCluster(dir_info->StartCluster, fs_info->ClusterSize, 0, buffer);
	uint32_t file_info_cluster = ((struct _FileNode*)buffer)->FileInfoCluster;
	ReadCluster(file_info_cluster, fs_info->ClusterSize, 0, data);
	uint32_t s_no_num = ((struct _FileInfo*)data)->FileSize;
	//计算当前文件夹使用到第几个个簇
	uint32_t now_used_cluster=0;
	if (s_no_num)
		now_used_cluster = CEIL(s_no_num, end_ptr - info->Index) - 1;
	//计算若加入新文件后需要的簇数
	uint32_t need_cluster = CEIL((s_no_num + 1) , (end_ptr - info->Index))-1;
	//若两个不相等则需要分配新的簇，否则直接写入
	int is_need_new_cluster = need_cluster - now_used_cluster;
	//定位到当前文件夹的最后一个簇
	uint32_t now_cluster = dir_info->StartCluster;
	unsigned int item_in_cluster_index;//新文件在簇中的索引
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
	//将新文件信息写入文件夹
	info->Index[item_in_cluster_index] = node_cluster;
	for (int i = 0; i < fs_info->ClusterSize; ++i)
	{
		WriteCluster(now_cluster, fs_info->ClusterSize, i, buffer + i * 512);
	}
	kfree(buffer);
	//更新文件夹信息
	((struct _FileInfo*)data)->FileSize += 1;
	WriteCluster(file_info_cluster, fs_info->ClusterSize, 0, data);
	return 0;
}

int RemoveFile(struct FileSystemInfo* fs_info,struct FileInfo* file)
{
	//获取目标文件信息
	char* data = kmalloc(fs_info->ClusterSize*512), other[512];
	ReadCluster(file->StartCluster, fs_info->ClusterSize, 0, data);
	ReadCluster(((struct _FileNode*)data)->FileInfoCluster, fs_info->ClusterSize, 0, data);
	//检查目标文件是否是文件夹
	if (((struct _FileInfo*)data)->FileType == FILE_TYPE_DIR)
	{
		//目标是文件夹
		//检查文件夹是否为空
		if (((struct _FileInfo*)data)->FileSize > 0)
		{
			kfree(data);
			return -1;//文件夹不为空
		}
	}
	//获取父节点信息
	uint32_t parent_node_cluster = ((struct _FileInfo*)data)->ParentNode;
	for (int i = 0; i < fs_info->ClusterSize; ++i)
		ReadCluster(parent_node_cluster, fs_info->ClusterSize, i, data + i * 512);
	uint32_t info_node = ((struct _FileNode*)data)->FileInfoCluster;
	ReadCluster(info_node, fs_info->ClusterSize, 0, other);
	uint32_t file_size = ((struct _FileInfo*)other)->FileSize;
	if (file_size == 0)
	{
		kfree(data);
		return -1;//文件为空，内部错误

	}

	//从父节点中删除目标文件
	//查找目标文件在父节点中的位置,将最后一个文件信息移动到目标文件位置，检查是否需要释放簇。
	//若需要释放簇，则修改倒数第二个文件信息的NextIndex为0
	uint32_t* end_ptr = (uint32_t*)(data + fs_info->ClusterSize * 512);
	struct _FileNode* info = (struct _FileInfo*)data;
	uint32_t now_cluster = parent_node_cluster;
	uint32_t now_used_cluster = file_size / (end_ptr - info->Index);
	uint32_t last_item_in_cluster_index = (file_size - 1) % (end_ptr - info->Index);
	uint32_t cluster_indexs_len = end_ptr - info->Index;
	uint32_t target_cluster = 0;
	uint32_t target_item_in_cluster_index = 0;
	//以下查找不会包括最后一次循环的簇
	uint32_t second_to_last_cluster = 0;//总是记录倒数第二个簇编号，以防需要释放最后一个簇
	for (uint32_t i = 0; i < now_used_cluster; ++i)
	{
		//在查找最后一个文件信息的同时，查找目标文件的位置
		for (int j = 0; j < cluster_indexs_len; ++j)
		{
			if (info->Index[j] == file->StartCluster)
			{
				//记录目标位置以便后续操作
				target_cluster = now_cluster;
				target_item_in_cluster_index = j;
			}
		}
		if (i == now_used_cluster - 1)
			second_to_last_cluster = now_cluster;//此时还在倒数第二个簇
		now_cluster = ((struct _FileNode*)data)->NextIndex;
		for (int j = 0; j < fs_info->ClusterSize; ++j)
			ReadCluster(now_cluster, fs_info->ClusterSize, j, data + j * 512);
	}
	if (target_cluster == 0)
	{
		//在最后一个簇中查找目标文件
		for (int j = 0; j < cluster_indexs_len; ++j)
		{
			if (info->Index[j] == file->StartCluster)
			{
				//记录目标位置以便后续操作
				target_cluster = now_cluster;
				target_item_in_cluster_index = j;
			}
		}
	}
	if (target_cluster == 0)
	{
		kfree(data);
		return -2;//未找到目标文件
	}
	if (now_used_cluster == 0)
	{
		//只有一个簇
		if (target_item_in_cluster_index != last_item_in_cluster_index)
		{
			info->Index[target_item_in_cluster_index] = info->Index[last_item_in_cluster_index];
			for (int i = 0; i < fs_info->ClusterSize; ++i)
				WriteCluster(target_cluster, fs_info->ClusterSize, i, data + i * 512);
		}
	}
	else
	{
		//有多个簇
		if (target_cluster != now_cluster)
		{
			//目标文件不在最后一个簇
			int t = info->Index[last_item_in_cluster_index];
			//读取目标所在簇
			for (int i = 0; i < fs_info->ClusterSize; ++i)
				ReadCluster(target_cluster, fs_info->ClusterSize, i, data + i * 512);
			info->Index[target_item_in_cluster_index] = t;
			//写入目标所在簇
			for (int i = 0; i < fs_info->ClusterSize; ++i)
				WriteCluster(target_cluster, fs_info->ClusterSize, i, data + i * 512);
			//判断最后一个簇是否需要释放
			if (last_item_in_cluster_index == 0)
			{
				//需要释放最后一个簇
				FreeCluster(fs_info, now_cluster);
				//修改倒数第二个簇的NextIndex为0
				ReadCluster(second_to_last_cluster, fs_info->ClusterSize, 0, data);
				((struct _FileNode*)data)->NextIndex = 0;
				WriteCluster(second_to_last_cluster, fs_info->ClusterSize, 0, data);
			}
		}
		else
		{
			//目标文件在最后一个簇
			if (target_item_in_cluster_index == last_item_in_cluster_index)
			{
				//是最后一个文件信息
				if (target_item_in_cluster_index == 0)
				{
					//释放该簇
					FreeCluster(fs_info, now_cluster);
					//修改倒数第二个簇的NextIndex为0
					ReadCluster(second_to_last_cluster, fs_info->ClusterSize, 0, data);
					((struct _FileNode*)data)->NextIndex = 0;
					WriteCluster(second_to_last_cluster, fs_info->ClusterSize, 0, data);
				}
				//否则无需任何修改
			}
			else
			{
				//在最后簇的不同位置，一定不需要释放最后一个簇
				info->Index[target_item_in_cluster_index] = info->Index[last_item_in_cluster_index];
				for (int i = 0; i < fs_info->ClusterSize; ++i)
					WriteCluster(target_cluster, fs_info->ClusterSize, i, data + i * 512);
			}
		}
	}
	//更新文件夹信息节点
	((struct _FileInfo*)other)->FileSize -= 1;
	WriteCluster(info_node, fs_info->ClusterSize, 0, other);
	//释放文件本身
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
//函数从指定文件中读取长度为buffer_size的数据到buffer中，读取偏移为offset
int ReadFile(struct FileSystemInfo* fs_info,struct FileInfo* file_info, char* buffer, uint32_t buffer_size, uint32_t offset)
{
	char data[512];
	//读取文件信息
	ReadCluster(file_info->StartCluster, fs_info->ClusterSize, 0, data);
	ReadCluster(((struct _FileNode*)data)->FileInfoCluster, fs_info->ClusterSize, 0, data);
	//检查文件类型
	if (((struct _FileInfo*)data)->FileType != FILE_TYPE_FILE) return -1;//目标不是文件
	//检查读取偏移是否超出文件大小
	if (offset >= ((struct _FileInfo*)data)->FileSize) return -2;//读取偏移超出文件大小
	//计算读取的长度
	uint32_t read_size = buffer_size;
	if (offset + buffer_size > ((struct _FileInfo*)data)->FileSize)
	{
		read_size = ((struct _FileInfo*)data)->FileSize - offset;
	}
	//计算起始位置位于第几簇
	uint32_t start_cluster = offset / (fs_info->ClusterSize * 512-sizeof(struct _FileNode));
	uint32_t start_byte = offset % (fs_info->ClusterSize * 512 - sizeof(struct _FileNode));
	//读取文件起始簇
	int now_cluster = file_info->StartCluster;
	ReadCluster(now_cluster, fs_info->ClusterSize, 0, data);
	//定位到要读取的簇
	for (int i = 0; i < start_cluster; ++i)
	{
		now_cluster = ((struct _FileNode*)data)->NextIndex;
		ReadCluster(now_cluster, fs_info->ClusterSize, 0, data);
	}
	//读取数据
	int readed = 0;
	//申请内存用于存放单个簇
	char* cluster_buffer = (char*)kmalloc(fs_info->ClusterSize * 512);
	while (readed<read_size)
	{
		//读取簇
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
	//读取文件信息
	ReadCluster(file_info->StartCluster, fs_info->ClusterSize, 0, data);
	uint32_t info_cluster = ((struct _FileNode*)data)->FileInfoCluster;
	ReadCluster(info_cluster, fs_info->ClusterSize, 0, info);
	//检查文件类型
	if (((struct _FileInfo*)info)->FileType != FILE_TYPE_FILE)
	{
		kfree(data);
		return -1;//目标不是文件
	}
	file_size = ((struct _FileInfo*)info)->FileSize;
	//定位到要写入的簇，若超出文件大小则需要分配新的簇
	uint32_t start_cluster = offset / (fs_info->ClusterSize * 512-sizeof(struct _FileNode));
	uint32_t start_byte = offset % (fs_info->ClusterSize * 512-sizeof(struct _FileNode));
	//读取文件起始簇
	int now_cluster = file_info->StartCluster;
	ReadCluster(now_cluster, fs_info->ClusterSize, 0, data);
	//定位到要写入的簇
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
	//读取剩余扇区
	for (int i = 1; i < fs_info->ClusterSize; ++i)
	{
		ReadCluster(now_cluster, fs_info->ClusterSize, i, data + i * 512);
	}
	//开始循环写入数据
	is_new_cluster = 0;
	while (writed < buffer_size)
	{
		//写入数据
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
			//写入簇
			for (int i = 0; i < 4; ++i)
			{
				WriteCluster(now_cluster, fs_info->ClusterSize, i, data + i * 512);
			}
		}
		//若写入完毕则退出
		if (writed == buffer_size)
		{
			WriteCluster(now_cluster, fs_info->ClusterSize, 0, data);
			break;
		}
		//检查下一簇是否存在,不存在则分配新的簇
		if (((struct _FileNode*)data)->NextIndex == 0)
		{
			//分配新的簇
			((struct _FileNode*)data)->NextIndex = AllocatCluster(fs_info);
			WriteCluster(now_cluster, fs_info->ClusterSize, 0, data);
			is_new_cluster = 1;
		}
		else is_new_cluster = 0;
		now_cluster = ((struct _FileNode*)data)->NextIndex;
		if (is_new_cluster==0)
		{
			//读取下一簇
			for (int i = 0; i < fs_info->ClusterSize; ++i)
			{
				ReadCluster(now_cluster, fs_info->ClusterSize, i, data + i * 512);
			}
		}
	}
	//更新文件信息
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
	if (((struct _FileInfo*)data)->FileType != FILE_TYPE_DIR) return -1;//目标不是文件夹
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
		return -1;//目标不是文件夹
	}
	if (index >= ((struct _FileInfo*)(data + 512))->FileSize)
	{
		kfree(data);
		return -2;//Out of bound
	}
	//计算位于第几簇
	uint32_t target_cluster = index / max_len;
	uint32_t target_index_in_cluster = index % max_len;
	
	uint32_t now_cluster = dir_info->StartCluster;
	for (int i = 0; i < target_cluster; ++i)
	{
		now_cluster = ((struct _FileNode*)(data))->NextIndex;
		ReadCluster(now_cluster, fs_info->ClusterSize, 0, data);
	}
	//将目标簇读取完整
	for (int i = 1; i < fs_info->ClusterSize; ++i)
		ReadCluster(now_cluster, fs_info->ClusterSize, i, data + i * 512);
	target_cluster = ((struct _FileNode*)data)->Index[target_index_in_cluster];
	//读取目标文件头
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
	if (*p != '/') return -1;//路径没有以/开始
	p += 1;
	//读取文件系统跟目录
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
				return -2;//路径或文件不存在
			}
			if (*p == 0) break;
			++p;
		}
	}
	*file_info = fp;
	return 0;
}
