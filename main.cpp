
extern "C" {
#include "filesystem.h"
}
#include <string.h>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <list>
#include <memory>
int main()
{
	std::cout << "虚拟文件系统 POWERED BY AllesUgo" << std::endl;
	std::cout << "输入help查看可用命令" << std::endl;
	std::cout << "操作文件系统前请先创建或打开虚拟磁盘并打开文件系统" << std::endl;
	FileSystemInfo fs;
	FileInfo current_dir_info;
	bool is_open_fs = false;
	std::list<std::pair<std::string,FileInfo>> current_dir;
	std::string cmd;
	bool is_open_vd = false;
	while (1)
	{
		try
		{
			if (is_open_fs == true)
			{
				bool first = true;
				for (auto& p : current_dir)
				{
					if (first!=true)
						std::cout  << p.first<<'/';
					else
					{
						std::cout << p.first;
						first = false;
					}
				}
			}
			std::cout << ">";
			std::getline(std::cin, cmd);
			std::istringstream iss(cmd);
			std::string command;
			iss >> command;
			if (command == "mkdir")
			{
				if (!is_open_fs)
				{
					throw std::runtime_error("No file system is open");
				}
				std::string path;
				iss >> path;
				if (0 != CreateFile(&fs, &current_dir_info, FILE_TYPE_DIR, path.c_str()))
					throw std::runtime_error("Create directory failed");
			}
			else if (command == "touch")
			{
				if (!is_open_fs)
				{
					throw std::runtime_error("No file system is open");
				}
				std::string path;
				iss >> path;
				if (0 != CreateFile(&fs, &current_dir_info, FILE_TYPE_FILE, path.c_str()))
					throw std::runtime_error("Create file failed");
			}
			else if (command == "cat")
			{
				if (!is_open_fs)
				{
					throw std::runtime_error("No file system is open");
				}
				std::string path;
				iss >> path;
				FileInfo file;
				if (0 != GetFileFromDir(&fs, &current_dir_info, path.c_str(), &file))
					throw std::runtime_error("No such file: " + path);
				if (file.Type != FILE_TYPE_FILE)
					throw std::runtime_error("Not a file");
				if (file.FileSize == 0)
				{
					std::cout << std::endl;
					continue;
				}
				std::unique_ptr<char[]> buffer = std::make_unique<char[]>(file.FileSize+1);
				if (ReadFile(&fs, &file, buffer.get(), file.FileSize, 0)<0)
					throw std::runtime_error("Read file failed");
				std::cout << buffer << std::endl;
			}
			else if (command == "write")
			{
				if (!is_open_fs)
				{
					throw std::runtime_error("No file system is open");
				}
				std::string path;
				iss >> path;
				FileInfo file;
				if (0 != GetFileFromDir(&fs, &current_dir_info, path.c_str(), &file))
					throw std::runtime_error("No such file: " + path);
				if (file.Type != FILE_TYPE_FILE)
					throw std::runtime_error("Not a file");
				std::string content;
				std::cout << "输入数据，直到输入一个#为止" << std::endl;
				while (1)
				{
					std::string line;
					std::getline(std::cin, line);
					if (line == "#")
						break;
					content += line + "\n";
				}
				if (0 != WriteFile(&fs, &file, content.c_str(), content.size(), 0))
					throw std::runtime_error("Write file failed");
			}
			else if (command == "ls")
			{
				if (!is_open_fs)
				{
					throw std::runtime_error("No file system is open");
				}
				std::string arg;
				iss >> arg;
				if (arg == "-l")
				{
					if (!is_open_fs)
					{
						throw std::runtime_error("No file system is open");
					}
					int n = GetDirFileNumber(&fs, &current_dir_info);
					for (int i = 0; i < n; i++)
					{
						FileInfo file;
						char buffer[1024];
						GetFileFromDirWithIndex(&fs, &current_dir_info, i, buffer, 1024, &file);
						std::cout << (file.Type == FILE_TYPE_DIR ? "d" : "-") << "\t";
						std::cout << file.FileSize << "\t";
						GetFileName(&fs, &file, buffer, 1024);
						std::cout << buffer << std::endl;
					}
				}
				else
				{
					int n = GetDirFileNumber(&fs, &current_dir_info);
					for (int i = 0; i < n; i++)
					{
						FileInfo file;
						char buffer[1024];
						GetFileFromDirWithIndex(&fs, &current_dir_info, i, buffer, 1024, &file);
						std::cout << buffer << " ";
					}
					std::cout << std::endl;
				}
			}
			else if (command == "cd")
			{
				if (!is_open_fs)
				{
					throw std::runtime_error("No file system is open");
				}
				std::string path;
				iss >> path;
				if (path == "..")
				{
					if (current_dir.size() == 1)
						throw std::runtime_error("Already at root directory");
					current_dir.pop_back();
					current_dir_info = current_dir.back().second;
				}
				else
				{
					FileInfo file;
					if (0 != GetFileFromDir(&fs, &current_dir_info, path.c_str(), &file))
						throw std::runtime_error("No such directory: " + path);
					if (file.Type != FILE_TYPE_DIR)
						throw std::runtime_error("Not a directory");
					current_dir.push_back(std::make_pair(path, file));
					current_dir_info = file;
				}
			}
			else if (command == "pwd")
			{
				if (!is_open_fs)
				{
					throw std::runtime_error("No file system is open");
				}
				for (auto& p : current_dir)
				{
					std::cout << p.first << "/";
				}
			}
			else if (command == "rm")
			{
				if (!is_open_fs)
				{
					throw std::runtime_error("No file system is open");
				}
				std::string path;
				iss >> path;
				FileInfo file;
				if (0 != GetFileFromDir(&fs, &current_dir_info, path.c_str(), &file))
					throw std::runtime_error("No such file: " + path);
				if (0 != RemoveFile(&fs, &file))
					throw std::runtime_error("Remove file failed");
			}
			else if (command == "dd")
			{
				//用于虚拟文件系统与外部系统之间拷贝文件
				//指令格式 dd (in|out) 输入文件 输出文件)
				if (!is_open_fs)
				{
					throw std::runtime_error("No file system is open");
				}
				std::string mode;
				std::string in_file;
				std::string out_file;
				iss >> mode >> in_file >> out_file;
				if (mode == "in")
				{
					FILE* in = fopen(in_file.c_str(), "rb");
					if (in == nullptr)
						throw std::runtime_error("Open file failed");
					FileInfo file;
					if (0 != CreateFile(&fs, &current_dir_info, FILE_TYPE_FILE, out_file.c_str()))
						throw std::runtime_error("Create file failed");
					if (0 != GetFileFromDir(&fs, &current_dir_info, out_file.c_str(), &file))
						throw std::runtime_error("Open file failed: " + out_file);
					std::unique_ptr <char[]> buffer = std::make_unique<char[]>(1024 * 1024);
					while (1)
					{
						int n = fread(buffer.get(), 1, 1024 * 1024, in);
						if (n <= 0)
							break;
						if (0 != WriteFile(&fs, &file, buffer.get(), n, file.FileSize))
							throw std::runtime_error("Write file failed");
						file.FileSize += n;
					}
					fclose(in);
				}
				else if (mode == "out")
				{
					FILE* out = fopen(out_file.c_str(), "wb");
					if (out == nullptr)
						throw std::runtime_error("Open file failed");
					FileInfo file;
					if (0 != GetFileFromDir(&fs, &current_dir_info, in_file.c_str(), &file))
						throw std::runtime_error("No such file: " + in_file);
					if (file.Type != FILE_TYPE_FILE)
						throw std::runtime_error("Not a file");
					std::unique_ptr <char[]> buffer = std::make_unique<char[]>(1024 * 1024);
					for (int i = 0; i < file.FileSize; i += 1024*1024)
					{
						int n = ReadFile(&fs, &file, buffer.get(), 1024 * 1024, i);
						if (n < 0)
							throw std::runtime_error("Read file failed");
						fwrite(buffer.get(), 1, n, out);
					}
					fclose(out);
				}
				else
				{
					throw std::runtime_error("Invalid mode");
				}
			}
			else if (command == "mkfs")
			{
				uint32_t start_sector;
				uint32_t cluster_count;
				iss >> start_sector >> cluster_count;
				if (is_open_vd == false)
				{
					throw std::runtime_error("No virtual disk is open");
				}
				if (0 != CreateFileSystem(start_sector, cluster_count, &fs))
					throw std::runtime_error("Create file system failed");
				if (0 != GetRootDir(&fs, &current_dir_info))
					throw std::runtime_error("Get root directory failed");
				current_dir.clear();
				current_dir.push_back(std::make_pair("/", current_dir_info));
				is_open_fs = true;
			}
			else if (command == "openfs")
			{
				uint32_t start_sector;
				iss >> start_sector;
				if (is_open_vd == false)
				{
					throw std::runtime_error("No virtual disk is open");
				}
				if (0 != OpenFileSystem(start_sector, &fs))
					throw std::runtime_error("Open file system failed");
				if (0 != GetRootDir(&fs, &current_dir_info))
					throw std::runtime_error("Get root directory failed");
				current_dir.clear();
				current_dir.push_back(std::make_pair("/", current_dir_info));
				is_open_fs = true;
				std::cout << "已打开文件系统，文件系统信息如下：" << std::endl;
				std::cout << "ClusterSize: " << fs.ClusterSize << std::endl;
				std::cout << "StartSector: " << fs.StartSector << std::endl;
				std::cout << "ClusterCount: " << fs.ClusterCount << std::endl;
				std::cout << "FileSystemSize: " << fs.ClusterCount * fs.ClusterSize*512 << std::endl;
			}
			else if (command == "closefs")
			{
				if (!is_open_fs)
				{
					throw std::runtime_error("No file system is open");
				}
				is_open_fs = false;
			}
			else if (command == "exit")
			{
				break;
			}
			else if (command == "mkvd")
			{
				std::string path;
				iss >> path;
				if (0 != MakeVDisk(path.c_str()))
					throw std::runtime_error("Make virtual disk failed");
			}
			else if (command == "openvd")
			{
				std::string path;
				iss >> path;
				if (0 != OpenVDisk(path.c_str()))
					throw std::runtime_error("Open virtual disk failed");
				is_open_vd = true;
				//关闭当前打开的文件系统
				is_open_fs = false;
			}
			else if (cmd == "help")
			{
				std::cout << "可用命令列表：\n";
				std::cout << "  mkdir <路径>          : 在指定路径创建一个目录\n";
				std::cout << "  touch <路径>          : 在指定路径创建一个文件\n";
				std::cout << "  cat <路径>            : 显示指定文件的内容\n";
				std::cout << "  write <路径>          : 向指定文件写入数据\n";
				std::cout << "  ls [-l]               : 列出当前目录中的文件和目录\n";
				std::cout << "  cd <路径>             : 切换到指定的目录\n";
				std::cout << "  pwd                   : 显示当前所在的目录路径\n";
				std::cout << "  rm <路径>             : 删除指定的文件或目录\n";
				std::cout << "  dd in <输入文件> <输出文件> : 将外部文件导入虚拟文件系统\n";
				std::cout << "  dd out <输入文件> <输出文件>: 将虚拟文件系统中的文件导出到外部\n";
				std::cout << "  mkfs <起始扇区> <簇数量>: 创建新的文件系统\n";
				std::cout << "  openfs <起始扇区>     : 打开已存在的文件系统\n";
				std::cout << "  closefs               : 关闭当前打开的文件系统\n";
				std::cout << "  mkvd <路径>           : 创建一个新的虚拟磁盘\n";
				std::cout << "  openvd <路径>         : 打开一个已存在的虚拟磁盘\n";
				std::cout << "  exit                  : 退出程序\n";
				std::cout << "  help                  : 显示此帮助信息\n";
				std::cout << "注意，任何虚拟文件系统中的路径都不支持多级，例如cd后只能跟一个文件夹的名字，不能跟a/b这样的路径\n" <<
					"dd in和dd out命令用于虚拟文件系统与外部系统之间拷贝文件，in表示将外部文件导入虚拟文件系统，out表示将虚拟文件系统中的文件导出到外部\n" <<
					"mkfs命令用于创建新的文件系统，需要指定文件系统的起始扇区和簇数量\n" <<
					"openfs命令用于打开已存在的文件系统，需要指定文件系统的起始扇区\n" <<
					"closefs命令用于关闭当前打开的文件系统" << std::endl;

			}

			else
			{
				std::cout << "Invalid command" << std::endl;
			}
		}
		catch (std::exception& e)
		{
			std::cout << e.what() << std::endl;
		}
	}
	End();
}


