#pragma once
#ifndef FILESYSTEM_H_
#define FILESYSTEM_H_

#include "try2.0.h"
#include <string>
//ȫ�ֱ�������
extern const int kSuperBlockStartAddress;																				//������ ƫ�Ƶ�ַ,ռһ�����̿�
extern const int kInodeBitmapStartAddress;																	//inodeλͼ ƫ�Ƶ�ַ��ռ512�����̿飬����� 524288 ��inode��״̬
extern const int kBlockBitmapStartAddress;	//blockλͼ ƫ�Ƶ�ַ��ռ1024�����̿飬�����1G��״̬
extern const int kInodeStartAddress;		//inode�ڵ��� ƫ�Ƶ�ַ��ռ 65536 �����̿�
extern const int kBlockStartAddress;				//block������ ƫ�Ƶ�ַ ��ռ BLOCK_NUM �����̿�
extern const int kSumSize;													//��������ļ���С
extern const int kFileMaxSize;								//�����ļ�����С


class FileSystem
{
public:
	FileSystem(FILE *w, FILE *r,bool is_mount);
	void Parser();//�����ȡ�����
	~FileSystem();
	int error_;																											//������Ϣ
private:
	void Format();																										//��ʽ��
	int ialloc();																										//����inode�ռ�
	int balloc();																										//����block�ռ�
	int DirectoryLookup(int previous_file, const char *name, bool type, int &posi, int &posj);//�����ļ���Ŀ¼
	void Mkdir(int parinoAddr, const char *name);//�����ļ���
	void Open();//���ļ�ϵͳ
	void Cd(int parinoAddr, const char *path);//�򿪵�ǰĿ¼
	void Move(int be_shared_folder, const char *be_shared_name, int shar_to_folder, int file_type);//���к���
	void AddFileToFolder(int folder_inode_adress, int file_inode_adress, const char* file_name, bool file_type);//��һ���ļ������������
	void bfree(int addr);	//���̿��ͷź���
	void ifree(int addr);	//�ͷ�i���������
	void PutInRecycle(int previous_file, const char *file_name, bool file_type);//�Ž�����վ
	void RestoreFromRecycle(const char *file_name, bool file_type);//��ԭ�ļ�
	void EmptyRecycle();	//��ջ���վ
	void win_cp_minfs(int parinoAddr, const char* name, const char* win_path);
	void minifs_cp_win(int parinoAddr, const char* name, const char* win_path);//minifs��win����
	void more(int parinoAddr);//��ҳ���txt
	void type_txt(int parinoAddr);  //��ʾ��ǰ�ļ����µ��ı��ļ�
	void att(int parinoAddr, const char *file);//��ʾ�ռ��ļ�����
	void _Find(const char *a, int parinoAddr, const char *b);//��������
	void find(const char *a);//Ѱ���ļ�
	void ls(int parinoAddr);//��ʾĿ¼�������ļ�
	void DelRecursion(int delAddress);//ɾ���ļ���
	void minifs_cp_minifs(int parinoAddr, const char *name, int parinoAddr2);
	void copy_block(int parinoAddr, int parinoAddr2);//�������̿�
	int root_directory_address_;																						//��Ŀ¼inode��ַ
	int recycle_directory_address_;
	int current_directory_address_;																						//��ǰĿ¼
	std::string  current_directory_path_;																				//��ǰ·��
	FILE* fw;																											//��������ļ� д�ļ�ָ��
	FILE* fr;																											//��������ļ� ���ļ�ָ��
	SuperBlock *super_block_;																							//������ָ��
};

#endif // FILESYSTE	M_H_

