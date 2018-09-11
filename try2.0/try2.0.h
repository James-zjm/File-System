#pragma once
#ifndef TRY_20_H_
#define TRY_20_H

#include <iostream>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <conio.h>
#include <windows.h>

#define BLOCK_SIZE	1024					//盘块大小为1kB
#define INODE_SIZE	128						//inode节点大小为128B。注：sizeof(Inode)不能超过该值
#define MAX_NAME_SIZE 28                    //最大文件名长度
#define INODE_NUM	524288					//inode节点数 每2kb一个节点数量
#define BLOCK_NUM	1048576					//块号数，1048576 * 1kB = 1g
#define BLOCKS_PER_GROUP	128				//空闲块堆栈大小，一个空闲堆栈最多能存多少个磁盘块地址.
#define TYPE_DIR	1						//目录标识
#define TYPE_FILE	0						//文件标识
#define FILESYSNAME	"MingOS.sys"			//虚拟磁盘文件名


//全局变量定义
extern const int kSuperBlockStartAddress;	//超级块 偏移地址,占一个磁盘块
extern const int kInodeBitmapStartAddress;	//inode位图 偏移地址，占512个磁盘块，最多监控 524288 个inode的状态
extern const int kBlockBitmapStartAddress;	//block位图 偏移地址，占1024个磁盘块，最多监控1G的状态
extern const int kInodeStartAddress;		//inode节点区 偏移地址，占 65536 个磁盘块
extern const int kBlockStartAddress;		//block数据区 偏移地址 ，占 BLOCK_NUM 个磁盘块
extern const int kSumSize;					//虚拟磁盘文件大小
extern const int kFileMaxSize;				//单个文件最大大小


struct SuperBlock							//超级块
{
										//系统信息
	unsigned int s_inode_num;				//4B,inode节点数，最多 524288
	unsigned int s_free_inode_num;		    //4B,空闲inode节点数
	int s_free[BLOCKS_PER_GROUP];			//512B,空闲块堆栈
	unsigned int s_block_num;				//4B,磁盘块块数，最多 1048576
										//统计
	unsigned int s_free_block_num;			//4B,空闲磁盘块数
	int s_free_addr;						//4B,空闲块堆栈指针
										//结构大小
	unsigned int s_block_size;				//4B,磁盘块大小
	unsigned int s_inode_size;				//4B,inode大小
	unsigned int s_super_block_size;		//4B,超级块大小
	unsigned int s_blocks_per_group;		//4B,每 block group 的block数量
										//磁盘分布
	int s_super_block_start_address;        //4B,super block起始地址
	int s_inode_bitmap_start_address;		//4B，inodeb itmap起始地址
	int s_block_bitmap_start_address;		//4B，block bitmap起始地址
	int s_inode_start_address;				//4B，inode起始地址
	int s_block_start_address;				//4B，block起始地址
};											//568个字节

struct Inode								//inode节点
{
										//inode信息
	unsigned int i_number;					//4B，inode标识（编号）
	unsigned int i_type;					//4B，inode类型：0文件，1目录
	unsigned int i_file_num;				//4B，该目录下有多少个文件
	unsigned int i_size;					//4B，文件大小，单位为字节（B）
										//inode 时间
	time_t  i_create_time;					//8B,inode创建时间
	time_t  i_last_change_time;				//8B,文件变动的时间
	time_t  i_last_open_time;				//8B,文件上一次打开的时间
										//inode回收站
	int i_pre_folder_adress;                //4B,删除之前的文件夹
										//inode储存
	//0表示没有使用，其他表示iblock地址
	int i_direct_block[10];					//40B,10个直接块。10*1024B = 10KB
	int i_indirect_block_1;					//4B,一级间接块。1024B / 4 * 1024 = 256 * 1024B = 256KB
	int i_indirect_block_2;					//4B,二级间接块。(1024B / 4) * (1024B / 4) * 1024B = 256 * 256 * 1024B = 65536KB = 64MB
	int i_indirect_block_3;					//4B,三级间接块。(1024B / 4) * (1024B / 4) * (1024B / 4) * 1024B = 256 * 256 * 256 * 1024B =  16G
};											//100B

struct DirItem								//目录项,一个磁盘块共有32个
{
	char itemName[MAX_NAME_SIZE];			//28B,目录或者文件名
	int inode_address;							//4B,目录项对应的inode节点地址
};											//32B

#endif // TRY_20_H_