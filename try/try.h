#pragma once
#include <iostream>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <conio.h>
#include <windows.h>

#define BLOCK_SIZE	1024						//块号大小为1kB
#define INODE_SIZE	128						//inode节点大小为128B。注：sizeof(Inode)不能超过该值
#define MAX_NAME_SIZE 28                    //最大文件名长度

#define INODE_NUM	524288						//inode节点数 2kb i节点数量,即文件数量
#define BLOCK_NUM	1048576					//块号数，1048576 * 1kB = 1g
#define BLOCKS_PER_GROUP	128				//空闲块堆栈大小，一个空闲堆栈最多能存多少个磁盘块地址

#define MODE_DIR	1					//目录标识
#define MODE_FILE	0					//文件标识

#define FILESYSNAME	"MingOS.sys"			//虚拟磁盘文件名

#define _CRT_SECURE_NO_WARNINGS 

//结构体声明
//超级块
struct SuperBlock {
	unsigned int s_INODE_NUM;				//inode节点数，最多 524288
	unsigned int s_free_INODE_NUM;		    //空闲inode节点数

	unsigned int s_BLOCK_NUM;				//磁盘块块数，最多 1048576

	unsigned int s_free_BLOCK_NUM;			//空闲磁盘块数
	int s_free_addr;						//空闲块堆栈指针
	int s_free[BLOCKS_PER_GROUP];			//空闲块堆栈

	unsigned short s_BLOCK_SIZE;			//磁盘块大小
	unsigned short s_INODE_SIZE;			//inode大小
	unsigned short s_SUPERBLOCK_SIZE;		//超级块大小
	unsigned short s_blocks_per_group;		//每 blockgroup 的block数量

	//磁盘分布
	int s_Superblock_StartAddr;
	int s_InodeBitmap_StartAddr;
	int s_BlockBitmap_StartAddr;
	int s_Inode_StartAddr;
	int s_Block_StartAddr;
};

//inode节点
struct Inode {
	unsigned short i_ino;					//inode标识（编号）
	unsigned short i_mode;					//类型：0文件，1目录
	unsigned short i_cnt;					//链接数。有多少文件名指向这个inode
	unsigned int i_size;					//文件大小，单位为字节（B）
	time_t  i_ctime;						//inode上一次变动的时间
	time_t  i_mtime;						//文件内容上一次变动的时间
	time_t  i_atime;						//文件上一次打开的时间
	int i_dirBlock[10];						//10个直接块。10*1kB = 10KB
	int i_indirBlock_1;						//一级间接块。指向block，1kB/4 * 1024 = 256 * 1024B = 256KB
	unsigned int i_indirBlock_2;			//二级间接块。(1024B/4)*(1024B/4) * 1024B = 256*256*1024B = 8192KB = 64MB
	unsigned int i_indirBlock_3;			//三级间接块。(1024B/4)*(1024B/4)*(1024B/4) * 1024B = 256*256*256*1024B = 1677216KB = 16384MB = 16G
};

//目录项
struct DirItem {							//32字节，一个磁盘块能存 1024/32=32 个目录项
	char itemName[MAX_NAME_SIZE];			//目录或者文件名
	int inodeAddr;							//目录项对应的inode节点地址
};

//全局变量声明
extern SuperBlock *superblock;
extern const int Superblock_StartAddr;		//超级块 偏移地址,占一个磁盘块
extern const int InodeBitmap_StartAddr;		//inode位图 偏移地址，占两个磁盘块，最多监控 1024 个inode的状态
extern const int BlockBitmap_StartAddr;		//block位图 偏移地址，占二十个磁盘块，最多监控 10240 个磁盘块（5120KB）的状态
extern const int Inode_StartAddr;			//inode节点区 偏移地址，占 INODE_NUM/(BLOCK_SIZE/INODE_SIZE) 个磁盘块
extern const int Block_StartAddr;			//block数据区 偏移地址 ，占 INODE_NUM 个磁盘块
extern const int File_Max_Size;				//单个文件最大大小
extern const int Sum_Size;					//虚拟磁盘文件大小


//全局变量声明
extern int Root_Dir_Addr;					//根目录inode地址
extern int Cur_Dir_Addr;					//当前目录
extern char Cur_Dir_Name[310];				//当前目录名

extern FILE* fw;							//虚拟磁盘文件 写文件指针
extern FILE* fr;							//虚拟磁盘文件 读文件指针
extern SuperBlock *superblock;				//超级块指针
extern bool inode_bitmap[INODE_NUM];		//inode位图
extern bool block_bitmap[BLOCK_NUM];		//磁盘块位图

//extern char buffer[10000000];				//10M，缓存整个虚拟磁盘文件


//------------------------------------------------------------
bool Format();   //格式化
int ialloc();	//分配i节点区函数，返回inode地址
int balloc();	//磁盘块分配函数
bool Install();  //安装文件系统，将虚拟磁盘文件中的关键信息如超级块读入到内存
void ls(int parinoAddr); //显示当前目录下的所有文件
void info();        //命令解释
bool win_cp_minfs(char *a, char *b);//win向minifs拷贝命令
bool minifs_cp_win(char *a, char *b);//minifs向win拷贝
bool minifs_cp_minifs(char *a, char *b);//minifs向minifs拷贝
bool del_file(int parinoAddr, char *a);//删除文件，名字
bool del_folder(int parinoAddr, char *a);//删除文件夹
void type_txt(int parinoAddr);//显示当前文件夹下的txt
void more(int parinoAddr);//分页显示
void att(char *b);//显示空间文件属性
void help();//显示帮助信息
bool quit();//退出系统
void find(char *a);//寻找文件
int find_n(int parinoAddr, char *a, int mode);//内部寻找文件，返回inode地址
void cd(int parinoAddr, char name[]);	//进入当前目录下的name目录