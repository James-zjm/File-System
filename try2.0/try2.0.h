#pragma once
#ifndef TRY_20_H_
#define TRY_20_H

#include <iostream>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <conio.h>
#include <windows.h>

#define BLOCK_SIZE	1024					//�̿��СΪ1kB
#define INODE_SIZE	128						//inode�ڵ��СΪ128B��ע��sizeof(Inode)���ܳ�����ֵ
#define MAX_NAME_SIZE 28                    //����ļ�������
#define INODE_NUM	524288					//inode�ڵ��� ÿ2kbһ���ڵ�����
#define BLOCK_NUM	1048576					//�������1048576 * 1kB = 1g
#define BLOCKS_PER_GROUP	128				//���п��ջ��С��һ�����ж�ջ����ܴ���ٸ����̿��ַ.
#define TYPE_DIR	1						//Ŀ¼��ʶ
#define TYPE_FILE	0						//�ļ���ʶ
#define FILESYSNAME	"MingOS.sys"			//��������ļ���


//ȫ�ֱ�������
extern const int kSuperBlockStartAddress;	//������ ƫ�Ƶ�ַ,ռһ�����̿�
extern const int kInodeBitmapStartAddress;	//inodeλͼ ƫ�Ƶ�ַ��ռ512�����̿飬����� 524288 ��inode��״̬
extern const int kBlockBitmapStartAddress;	//blockλͼ ƫ�Ƶ�ַ��ռ1024�����̿飬�����1G��״̬
extern const int kInodeStartAddress;		//inode�ڵ��� ƫ�Ƶ�ַ��ռ 65536 �����̿�
extern const int kBlockStartAddress;		//block������ ƫ�Ƶ�ַ ��ռ BLOCK_NUM �����̿�
extern const int kSumSize;					//��������ļ���С
extern const int kFileMaxSize;				//�����ļ�����С


struct SuperBlock							//������
{
										//ϵͳ��Ϣ
	unsigned int s_inode_num;				//4B,inode�ڵ�������� 524288
	unsigned int s_free_inode_num;		    //4B,����inode�ڵ���
	int s_free[BLOCKS_PER_GROUP];			//512B,���п��ջ
	unsigned int s_block_num;				//4B,���̿��������� 1048576
										//ͳ��
	unsigned int s_free_block_num;			//4B,���д��̿���
	int s_free_addr;						//4B,���п��ջָ��
										//�ṹ��С
	unsigned int s_block_size;				//4B,���̿��С
	unsigned int s_inode_size;				//4B,inode��С
	unsigned int s_super_block_size;		//4B,�������С
	unsigned int s_blocks_per_group;		//4B,ÿ block group ��block����
										//���̷ֲ�
	int s_super_block_start_address;        //4B,super block��ʼ��ַ
	int s_inode_bitmap_start_address;		//4B��inodeb itmap��ʼ��ַ
	int s_block_bitmap_start_address;		//4B��block bitmap��ʼ��ַ
	int s_inode_start_address;				//4B��inode��ʼ��ַ
	int s_block_start_address;				//4B��block��ʼ��ַ
};											//568���ֽ�

struct Inode								//inode�ڵ�
{
										//inode��Ϣ
	unsigned int i_number;					//4B��inode��ʶ����ţ�
	unsigned int i_type;					//4B��inode���ͣ�0�ļ���1Ŀ¼
	unsigned int i_file_num;				//4B����Ŀ¼���ж��ٸ��ļ�
	unsigned int i_size;					//4B���ļ���С����λΪ�ֽڣ�B��
										//inode ʱ��
	time_t  i_create_time;					//8B,inode����ʱ��
	time_t  i_last_change_time;				//8B,�ļ��䶯��ʱ��
	time_t  i_last_open_time;				//8B,�ļ���һ�δ򿪵�ʱ��
										//inode����վ
	int i_pre_folder_adress;                //4B,ɾ��֮ǰ���ļ���
										//inode����
	//0��ʾû��ʹ�ã�������ʾiblock��ַ
	int i_direct_block[10];					//40B,10��ֱ�ӿ顣10*1024B = 10KB
	int i_indirect_block_1;					//4B,һ����ӿ顣1024B / 4 * 1024 = 256 * 1024B = 256KB
	int i_indirect_block_2;					//4B,������ӿ顣(1024B / 4) * (1024B / 4) * 1024B = 256 * 256 * 1024B = 65536KB = 64MB
	int i_indirect_block_3;					//4B,������ӿ顣(1024B / 4) * (1024B / 4) * (1024B / 4) * 1024B = 256 * 256 * 256 * 1024B =  16G
};											//100B

struct DirItem								//Ŀ¼��,һ�����̿鹲��32��
{
	char itemName[MAX_NAME_SIZE];			//28B,Ŀ¼�����ļ���
	int inode_address;							//4B,Ŀ¼���Ӧ��inode�ڵ��ַ
};											//32B

#endif // TRY_20_H_