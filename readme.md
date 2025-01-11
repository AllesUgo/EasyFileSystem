# Easy File System
一个简单的链接式文件系统  
文件系统依赖的四个函数分别是向指定扇区写入数据，从指定扇区读取数据，向指定扇区写入数据，从指定扇区读取数据，申请内存空间，释放内存空间。  
只需要为项目提供这四个函数的实现，就可以使用这个文件系统。
basic_io.h是文件系统的头文件，包含上述的几个函数的声明。  
filesystem.h是文件系统的头文件，包含文件系统的接口。  
filesystem.c是文件系统的实现文件，包含文件系统的实现。  
当前的整个项目实现了一个虚拟文件系统，用一个文件模拟磁盘，文件系统的数据结构存储在文件中。