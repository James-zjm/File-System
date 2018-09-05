#pragma once
#include <iostream>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <conio.h>
#include <windows.h>

#define BLOCK_SIZE	1024						//��Ŵ�СΪ1kB
#define INODE_SIZE	128						//inode�ڵ��СΪ128B��ע��sizeof(Inode)���ܳ�����ֵ
#define MAX_NAME_SIZE 28                    //����ļ�������

#define INODE_NUM	524288						//inode�ڵ��� 2kb i�ڵ�����,���ļ�����
#define BLOCK_NUM	1048576					//�������1048576 * 1kB = 1g
#define BLOCKS_PER_GROUP	128				//���п��ջ��С��һ�����ж�ջ����ܴ���ٸ����̿��ַ

#define MODE_DIR	1					//Ŀ¼��ʶ
#define MODE_FILE	0					//�ļ���ʶ

#define FILESYSNAME	"MingOS.sys"			//��������ļ���

#define _CRT_SECURE_NO_WARNINGS 

//�ṹ������
//������
struct SuperBlock {
	unsigned int s_INODE_NUM;				//inode�ڵ�������� 524288
	unsigned int s_free_INODE_NUM;		    //����inode�ڵ���

	unsigned int s_BLOCK_NUM;				//���̿��������� 1048576

	unsigned int s_free_BLOCK_NUM;			//���д��̿���
	int s_free_addr;						//���п��ջָ��
	int s_free[BLOCKS_PER_GROUP];			//���п��ջ

	unsigned short s_BLOCK_SIZE;			//���̿��С
	unsigned short s_INODE_SIZE;			//inode��С
	unsigned short s_SUPERBLOCK_SIZE;		//�������С
	unsigned short s_blocks_per_group;		//ÿ blockgroup ��block����

	//���̷ֲ�
	int s_Superblock_StartAddr;
	int s_InodeBitmap_StartAddr;
	int s_BlockBitmap_StartAddr;
	int s_Inode_StartAddr;
	int s_Block_StartAddr;
};

//inode�ڵ�
struct Inode {
	unsigned short i_ino;					//inode��ʶ����ţ�
	unsigned short i_mode;					//���ͣ�0�ļ���1Ŀ¼
	unsigned short i_cnt;					//���������ж����ļ���ָ�����inode
	unsigned int i_size;					//�ļ���С����λΪ�ֽڣ�B��
	time_t  i_ctime;						//inode��һ�α䶯��ʱ��
	time_t  i_mtime;						//�ļ�������һ�α䶯��ʱ��
	time_t  i_atime;						//�ļ���һ�δ򿪵�ʱ��
	int i_dirBlock[10];						//10��ֱ�ӿ顣10*1kB = 10KB
	int i_indirBlock_1;						//һ����ӿ顣ָ��block��1kB/4 * 1024 = 256 * 1024B = 256KB
	unsigned int i_indirBlock_2;			//������ӿ顣(1024B/4)*(1024B/4) * 1024B = 256*256*1024B = 8192KB = 64MB
	unsigned int i_indirBlock_3;			//������ӿ顣(1024B/4)*(1024B/4)*(1024B/4) * 1024B = 256*256*256*1024B = 1677216KB = 16384MB = 16G
};

//Ŀ¼��
struct DirItem {							//32�ֽڣ�һ�����̿��ܴ� 1024/32=32 ��Ŀ¼��
	char itemName[MAX_NAME_SIZE];			//Ŀ¼�����ļ���
	int inodeAddr;							//Ŀ¼���Ӧ��inode�ڵ��ַ
};

//ȫ�ֱ�������
extern SuperBlock *superblock;
extern const int Superblock_StartAddr;		//������ ƫ�Ƶ�ַ,ռһ�����̿�
extern const int InodeBitmap_StartAddr;		//inodeλͼ ƫ�Ƶ�ַ��ռ�������̿飬����� 1024 ��inode��״̬
extern const int BlockBitmap_StartAddr;		//blockλͼ ƫ�Ƶ�ַ��ռ��ʮ�����̿飬����� 10240 �����̿飨5120KB����״̬
extern const int Inode_StartAddr;			//inode�ڵ��� ƫ�Ƶ�ַ��ռ INODE_NUM/(BLOCK_SIZE/INODE_SIZE) �����̿�
extern const int Block_StartAddr;			//block������ ƫ�Ƶ�ַ ��ռ INODE_NUM �����̿�
extern const int File_Max_Size;				//�����ļ�����С
extern const int Sum_Size;					//��������ļ���С


//ȫ�ֱ�������
extern int Root_Dir_Addr;					//��Ŀ¼inode��ַ
extern int Cur_Dir_Addr;					//��ǰĿ¼
extern char Cur_Dir_Name[310];				//��ǰĿ¼��

extern FILE* fw;							//��������ļ� д�ļ�ָ��
extern FILE* fr;							//��������ļ� ���ļ�ָ��
extern SuperBlock *superblock;				//������ָ��
extern bool inode_bitmap[INODE_NUM];		//inodeλͼ
extern bool block_bitmap[BLOCK_NUM];		//���̿�λͼ

//extern char buffer[10000000];				//10M������������������ļ�


//------------------------------------------------------------
bool Format();   //��ʽ��
int ialloc();	//����i�ڵ�������������inode��ַ
int balloc();	//���̿���亯��
bool Install();  //��װ�ļ�ϵͳ������������ļ��еĹؼ���Ϣ�糬������뵽�ڴ�
void ls(int parinoAddr); //��ʾ��ǰĿ¼�µ������ļ�
void info();        //�������
bool win_cp_minfs(char *a, char *b);//win��minifs��������
bool minifs_cp_win(char *a, char *b);//minifs��win����
bool minifs_cp_minifs(char *a, char *b);//minifs��minifs����
bool del_file(int parinoAddr, char *a);//ɾ���ļ�������
bool del_folder(int parinoAddr, char *a);//ɾ���ļ���
void type_txt(int parinoAddr);//��ʾ��ǰ�ļ����µ�txt
void more(int parinoAddr);//��ҳ��ʾ
void att(char *b);//��ʾ�ռ��ļ�����
void help();//��ʾ������Ϣ
bool quit();//�˳�ϵͳ
void find(char *a);//Ѱ���ļ�
int find_n(int parinoAddr, char *a, int mode);//�ڲ�Ѱ���ļ�������inode��ַ
void cd(int parinoAddr, char name[]);	//���뵱ǰĿ¼�µ�nameĿ¼