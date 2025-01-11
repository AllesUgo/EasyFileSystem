
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
	std::cout << "�����ļ�ϵͳ POWERED BY AllesUgo" << std::endl;
	std::cout << "����help�鿴��������" << std::endl;
	std::cout << "�����ļ�ϵͳǰ���ȴ������������̲����ļ�ϵͳ" << std::endl;
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
				std::cout << "�������ݣ�ֱ������һ��#Ϊֹ" << std::endl;
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
				//���������ļ�ϵͳ���ⲿϵͳ֮�俽���ļ�
				//ָ���ʽ dd (in|out) �����ļ� ����ļ�)
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
				std::cout << "�Ѵ��ļ�ϵͳ���ļ�ϵͳ��Ϣ���£�" << std::endl;
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
				//�رյ�ǰ�򿪵��ļ�ϵͳ
				is_open_fs = false;
			}
			else if (cmd == "help")
			{
				std::cout << "���������б�\n";
				std::cout << "  mkdir <·��>          : ��ָ��·������һ��Ŀ¼\n";
				std::cout << "  touch <·��>          : ��ָ��·������һ���ļ�\n";
				std::cout << "  cat <·��>            : ��ʾָ���ļ�������\n";
				std::cout << "  write <·��>          : ��ָ���ļ�д������\n";
				std::cout << "  ls [-l]               : �г���ǰĿ¼�е��ļ���Ŀ¼\n";
				std::cout << "  cd <·��>             : �л���ָ����Ŀ¼\n";
				std::cout << "  pwd                   : ��ʾ��ǰ���ڵ�Ŀ¼·��\n";
				std::cout << "  rm <·��>             : ɾ��ָ�����ļ���Ŀ¼\n";
				std::cout << "  dd in <�����ļ�> <����ļ�> : ���ⲿ�ļ����������ļ�ϵͳ\n";
				std::cout << "  dd out <�����ļ�> <����ļ�>: �������ļ�ϵͳ�е��ļ��������ⲿ\n";
				std::cout << "  mkfs <��ʼ����> <������>: �����µ��ļ�ϵͳ\n";
				std::cout << "  openfs <��ʼ����>     : ���Ѵ��ڵ��ļ�ϵͳ\n";
				std::cout << "  closefs               : �رյ�ǰ�򿪵��ļ�ϵͳ\n";
				std::cout << "  mkvd <·��>           : ����һ���µ��������\n";
				std::cout << "  openvd <·��>         : ��һ���Ѵ��ڵ��������\n";
				std::cout << "  exit                  : �˳�����\n";
				std::cout << "  help                  : ��ʾ�˰�����Ϣ\n";
				std::cout << "ע�⣬�κ������ļ�ϵͳ�е�·������֧�ֶ༶������cd��ֻ�ܸ�һ���ļ��е����֣����ܸ�a/b������·��\n" <<
					"dd in��dd out�������������ļ�ϵͳ���ⲿϵͳ֮�俽���ļ���in��ʾ���ⲿ�ļ����������ļ�ϵͳ��out��ʾ�������ļ�ϵͳ�е��ļ��������ⲿ\n" <<
					"mkfs�������ڴ����µ��ļ�ϵͳ����Ҫָ���ļ�ϵͳ����ʼ�����ʹ�����\n" <<
					"openfs�������ڴ��Ѵ��ڵ��ļ�ϵͳ����Ҫָ���ļ�ϵͳ����ʼ����\n" <<
					"closefs�������ڹرյ�ǰ�򿪵��ļ�ϵͳ" << std::endl;

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


