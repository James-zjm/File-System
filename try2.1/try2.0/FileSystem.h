#pragma once
#ifndef FILESYSTEM_H_
#define FILESYSTEM_H_

#include "try2.0.h"
#include <string>
//全局变量定义
extern const int kSuperBlockStartAddress;																				//超级块 偏移地址,占一个磁盘块
extern const int kInodeBitmapStartAddress;																	//inode位图 偏移地址，占512个磁盘块，最多监控 524288 个inode的状态
extern const int kBlockBitmapStartAddress;	//block位图 偏移地址，占1024个磁盘块，最多监控1G的状态
extern const int kInodeStartAddress;		//inode节点区 偏移地址，占 65536 个磁盘块
extern const int kBlockStartAddress;				//block数据区 偏移地址 ，占 BLOCK_NUM 个磁盘块
extern const int kSumSize;													//虚拟磁盘文件大小
extern const int kFileMaxSize;								//单个文件最大大小


class FileSystem
{
public:
	FileSystem(FILE *w, FILE *r,bool is_mount);
	void Parser();//命令读取与解释
	~FileSystem();
	int error_;																											//错误信息
private:
	void Format();																										//格式化
	int ialloc();																										//分配inode空间
	int balloc();																										//分配block空间
	int DirectoryLookup(int previous_file, const char *name, bool type, int &posi, int &posj);//查找文件内目录
	void Mkdir(int parinoAddr, const char *name);//创建文件夹
	void Open();//打开文件系统
	void Cd(int parinoAddr, const char *path);//打开当前目录
	void Move(int be_shared_folder, const char *be_shared_name, int shar_to_folder, int file_type);//剪切函数
	void AddFileToFolder(int folder_inode_adress, int file_inode_adress, const char* file_name, bool file_type);//向一个文件夹中添加内容
	void bfree(int addr);	//磁盘块释放函数
	void ifree(int addr);	//释放i结点区函数
	void PutInRecycle(int previous_file, const char *file_name, bool file_type);//放进回收站
	void RestoreFromRecycle(const char *file_name, bool file_type);//还原文件
	void EmptyRecycle();	//清空回收站
	void win_cp_minfs(int parinoAddr, const char* name, const char* win_path);
	void minifs_cp_win(int parinoAddr, const char* name, const char* win_path);//minifs向win拷贝
	void more(int parinoAddr);//分页输出txt
	void type_txt(int parinoAddr);  //显示当前文件夹下的文本文件
	void att(int parinoAddr, const char *file);//显示空间文件属性
	void _Find(const char *a, int parinoAddr, const char *b);//查找内容
	void find(const char *a);//寻找文件
	void ls(int parinoAddr);//显示目录下所有文件
	void DelRecursion(int delAddress);//删除文件夹
	void minifs_cp_minifs(int parinoAddr, const char *name, int parinoAddr2);
	void copy_block(int parinoAddr, int parinoAddr2);//拷贝磁盘块
	int root_directory_address_;																						//根目录inode地址
	int recycle_directory_address_;
	int current_directory_address_;																						//当前目录
	std::string  current_directory_path_;																				//当前路径
	FILE* fw;																											//虚拟磁盘文件 写文件指针
	FILE* fr;																											//虚拟磁盘文件 读文件指针
	SuperBlock *super_block_;																							//超级块指针
};

#endif // FILESYSTE	M_H_

