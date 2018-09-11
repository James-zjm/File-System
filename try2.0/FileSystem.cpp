#include "pch.h"
#include "FileSystem.h"

bool inode_bitmap[INODE_NUM];																						//inodeλͼ
bool block_bitmap[BLOCK_NUM];																						//���̿�λͼ

FileSystem::FileSystem(FILE *w, FILE *r, bool is_mount):fw(w),fr(r)
{
	super_block_ = new SuperBlock;
	error_ = 0;
	if (is_mount == 0)
	{
		//��Ŀ¼inode��ַ ����ǰĿ¼��ַ������	
		printf("�ļ�ϵͳ���ڸ�ʽ������\n");
		Format();
		root_directory_address_ = kInodeStartAddress;			//��һ��inode��ַ
		current_directory_address_ = root_directory_address_;
		current_directory_path_ = "/";
		if (error_ != 0)
			return;
		printf("��ʽ�����\n");
		printf("����������е�һ�ε�½\n");
		system("pause");
		system("cls");
		Open();//���ļ�ϵͳ
	}
	else 
	{
		Open();//���ļ�ϵͳ
	}
}

FileSystem::~FileSystem()
{
}

void FileSystem::Format()	//��ʽ��һ����������ļ�
{
	int i, j;

	//��ʼ��������
	super_block_->s_inode_num = INODE_NUM;
	super_block_->s_block_num = BLOCK_NUM;
	super_block_->s_super_block_size = sizeof(SuperBlock);//568
	super_block_->s_inode_size = INODE_SIZE;
	super_block_->s_block_size = BLOCK_SIZE;
	super_block_->s_free_inode_num = INODE_NUM;
	super_block_->s_free_block_num = BLOCK_NUM;
	super_block_->s_blocks_per_group = BLOCKS_PER_GROUP;
	super_block_->s_free_addr = kBlockStartAddress;	//���п��ջָ��Ϊ��һ��block
	super_block_->s_super_block_start_address = kSuperBlockStartAddress;
	super_block_->s_block_bitmap_start_address = kBlockBitmapStartAddress;
	super_block_->s_inode_bitmap_start_address = kInodeBitmapStartAddress;
	super_block_->s_block_start_address = kBlockStartAddress;
	super_block_->s_inode_start_address = kInodeStartAddress;
	//���п��ջ�ں��渳ֵ

	//��ʼ��inodeλͼ
	memset(inode_bitmap, 0, sizeof(inode_bitmap));
	//��ʼ��blockλͼ
	memset(block_bitmap, 0, sizeof(block_bitmap));
	//��ʼ�����̿��������ݳ������ӷ���֯	
	i = BLOCK_NUM / BLOCKS_PER_GROUP - 1;
	for (; i >= 0; i--)
	{	//һ��INODE_NUM/BLOCKS_PER_GROUP 8192 �飬һ��FREESTACKNUM��128�������̿� ����һ�����̿���Ϊ����
		if (i == BLOCK_NUM / BLOCKS_PER_GROUP - 1)
			super_block_->s_free[0] = -1;	//û����һ�����п���
		else
			super_block_->s_free[0] = kBlockStartAddress + (i + 1)*BLOCKS_PER_GROUP*BLOCK_SIZE;	//ָ����һ�����п�
		for (j = 1; j < BLOCKS_PER_GROUP; j++)
		{
			super_block_->s_free[j] = kBlockStartAddress + (i*BLOCKS_PER_GROUP + j)*BLOCK_SIZE;
		}
		fseek(fw, kBlockStartAddress + i * BLOCKS_PER_GROUP*BLOCK_SIZE, SEEK_SET);
		fwrite(super_block_->s_free, sizeof(super_block_->s_free), 1, fw);	//д��������̿飬128*�ֽ�
		super_block_->s_free_block_num--;
		block_bitmap[i * BLOCKS_PER_GROUP] = 1;
	}

	//������д�뵽��������ļ�
	fseek(fw, kSuperBlockStartAddress, SEEK_SET);
	fwrite(super_block_, sizeof(SuperBlock), 1, fw);
	fflush(fw);//ǿ�Ƚ��������ڵ�����д�ز���stream ָ�����ļ���
	//д��inodeλͼ
	fseek(fw, kInodeBitmapStartAddress, SEEK_SET);
	fwrite(inode_bitmap, sizeof(inode_bitmap), 1, fw);
	fflush(fw);//ǿ�Ƚ��������ڵ�����д�ز���stream ָ�����ļ���
	//д��blockλͼ
	fseek(fw, kBlockBitmapStartAddress, SEEK_SET);
	fwrite(block_bitmap, sizeof(block_bitmap), 1, fw);
	fflush(fw);//ǿ�Ƚ��������ڵ�����д�ز���stream ָ�����ļ���

	//������Ŀ¼ "/"
	Inode cur;
	//����inode
	int inoAddr = ialloc();
	//�����inode������̿�
	int blockAddr = balloc();
	if (error_ != 0)
		return;
	//��������̿������һ����Ŀ "."
	DirItem dirlist[32] = { 0 };
	strcpy(dirlist[0].itemName, ".");
	dirlist[0].inode_address = inoAddr;

	//д�ش��̿�
	fseek(fw, blockAddr, SEEK_SET);
	fwrite(dirlist, sizeof(dirlist), 1, fw);

	//��inode��ֵ
	cur.i_number = 0;
	cur.i_create_time = time(NULL);
	cur.i_last_change_time = time(NULL);
	cur.i_last_open_time = time(NULL);
	cur.i_file_num = 1;	//һ�����ǰĿ¼,"."��Ŀ¼
	cur.i_direct_block[0] = blockAddr;
	for (i = 1; i < 10; i++) 
	{
		cur.i_direct_block[i] = 0;
	}
	cur.i_size = super_block_->s_block_size;
	cur.i_indirect_block_1 = 0;	//ûʹ��һ����ӿ�
	cur.i_indirect_block_2 = 0;
	cur.i_indirect_block_3 = 0;
	cur.i_type = TYPE_DIR;

	//д��inode
	fseek(fw, inoAddr, SEEK_SET);
	fwrite(&cur, sizeof(Inode), 1, fw);
	fflush(fw);
	//���õ�ַ

	//��������վ
	Mkdir(root_directory_address_, "Recycle");
	return;
}

int FileSystem::ialloc()	//����i�ڵ�������������inode��ַ
{
	//��inodeλͼ��˳����ҿ��е�inode���ҵ��򷵻�inode��ַ������������
	if (super_block_->s_free_inode_num == 0)
	{
		printf("û�п���inode���Է���\n");
		error_ = 3;
		return -1;
	}
	else
	{
		//˳����ҿ��е�inode
		int i;
		for (i = 0; i < super_block_->s_inode_num; i++)
		{
			if (inode_bitmap[i] == 0)	//�ҵ�����inode
				break;
		}
		//���³�����
		super_block_->s_free_inode_num--;	//����inode��-1
		fseek(fw, kSuperBlockStartAddress, SEEK_SET);
		fwrite(super_block_, sizeof(SuperBlock), 1, fw);

		//����inodeλͼ
		inode_bitmap[i] = 1;
		fseek(fw, kInodeBitmapStartAddress + i, SEEK_SET);
		fwrite(&inode_bitmap[i], sizeof(bool), 1, fw);
		fflush(fw);

		return kInodeStartAddress + i * super_block_->s_inode_size;
	}
}

int FileSystem::balloc()	//���̿���亯��
{
	//ʹ�ó������еĿ��п��ջ
	//���㵱ǰջ��
	int top;	//ջ��ָ��
	if (super_block_->s_free_block_num == 0)
	{	//ʣ����п���Ϊ0
		error_ = 4;
		return -1;	//û�пɷ���Ŀ��п飬����-1
	}
	else 
	{	//����ʣ���
		top = (super_block_->s_free_block_num - 1) % super_block_->s_blocks_per_group;//�����ջ��ֻʣһ����top��Ϊ0
	}
	//��ջ��ȡ��
	//�������ջ�ף�����ǰ��ŵ�ַ���أ���Ϊջ�׿�ţ�����ջ��ָ����¿��п��ջ����ԭ����ջ
	int retAddr;

	if (top == 0)
	{
		retAddr = super_block_->s_free_addr;
		super_block_->s_free_addr = super_block_->s_free[0];	//ȡ����һ�����п��п��ջ�Ŀ��п��λ�ã����¿��п��ջָ��

		//ȡ����Ӧ���п����ݣ�����ԭ���Ŀ��п��ջ
		//ȡ����һ�����п��ջ������ԭ����
		fseek(fr, super_block_->s_free_addr, SEEK_SET);
		fread(super_block_->s_free, sizeof(super_block_->s_free), 1, fr);
		fflush(fr);
		super_block_->s_free_block_num--;
	}
	else
	{	//�����Ϊջ�ף���ջ��ָ��ĵ�ַ���أ�ջ��ָ��-1.
		retAddr = super_block_->s_free[top];	//���淵�ص�ַ
		super_block_->s_free[top] = -1;	//��ջ��
		top--;		//ջ��ָ��-1
		super_block_->s_free_block_num--;	//���п���-1

	}

	//���³�����
	fseek(fw, kSuperBlockStartAddress, SEEK_SET);
	fwrite(super_block_, sizeof(SuperBlock), 1, fw);
	fflush(fw);

	//����blockλͼ
	block_bitmap[(retAddr - kBlockStartAddress) / BLOCK_SIZE] = 1;
	fseek(fw, (retAddr - kBlockStartAddress) / BLOCK_SIZE + kBlockBitmapStartAddress, SEEK_SET);	//(retAddr-Block_StartAddr)/BLOCK_SIZEΪ�ڼ������п�
	fwrite(&block_bitmap[(retAddr - kBlockStartAddress) / BLOCK_SIZE], sizeof(bool), 1, fw);
	fflush(fw);

	return retAddr;
}

void FileSystem::Mkdir(int previous_file, const char *name)	//Ŀ¼������������������һ��Ŀ¼�ļ�inode��ַ ,Ҫ������Ŀ¼��
{
	int chiinoAddr = ialloc();	//���䵱ǰ�ڵ��ַ 
	//��������Ŀ��inode
	Inode p;
	p.i_number = (chiinoAddr - kInodeStartAddress) / super_block_->s_inode_size;
	p.i_create_time = time(NULL);
	p.i_last_change_time = time(NULL);
	p.i_last_open_time = time(NULL);
	p.i_file_num = 2;	//�������ǰĿ¼,"."��".."
	//�������inode�Ĵ��̿飬�ڴ��̺���д��������¼ . �� ..
	int nextblockAddr = balloc();
	if (error_ != 0)
	{
		return;
	}
	DirItem dirlist2[32] = { 0 };	//��ʱĿ¼���б� - 2
	strcpy(dirlist2[0].itemName, ".");
	strcpy(dirlist2[1].itemName, "..");
	dirlist2[0].inode_address = chiinoAddr;	//��ǰĿ¼inode��ַ
	dirlist2[1].inode_address = previous_file;	//��Ŀ¼inode��ַ

	//д�뵽��ǰĿ¼�Ĵ��̿�
	fseek(fw, nextblockAddr, SEEK_SET);
	fwrite(dirlist2, sizeof(dirlist2), 1, fw);

	p.i_direct_block[0] = nextblockAddr;
	int k;
	for (k = 1; k < 10; k++)
	{
		p.i_direct_block[k] = 0;
	}
	p.i_size = super_block_->s_block_size;
	p.i_indirect_block_1 = 0;	//ûʹ��һ����ӿ�
	p.i_indirect_block_2 = 0;
	p.i_indirect_block_3 = 0;
	p.i_type = TYPE_DIR;

	//��inodeд�뵽�����inode��ַ
	fseek(fw, chiinoAddr, SEEK_SET);
	fwrite(&p, sizeof(Inode), 1, fw);
	AddFileToFolder(previous_file, chiinoAddr, name, p.i_type);
	if (error_ != 0)
	{
		bfree(nextblockAddr);
		ifree(chiinoAddr);
	}
/*	if (strlen(name) >= MAX_NAME_SIZE)
	{
		error_ = 6;
		return;
	}
	int i;
	DirItem dirlist[32];	//��ʱĿ¼�嵥
	Inode cur;
	fseek(fr, previous_file, SEEK_SET);
	fread(&cur, sizeof(Inode), 1, fr);
	if (cur.i_file_num == 320)
	{
		error_ = 7;
		return;
	}

	int posi = -1, posj = -1;
	DirectoryLookup(previous_file, name, TYPE_DIR, posi, posj);
	if (posi != -1 && posj != -1)//���ҵ�ͬ���ļ�
	{
		error_ = 8;
		return;
	}
	DirectoryLookup(previous_file, "", TYPE_DIR, posi, posj);

	if (posi != -1&&posj!=-1)
	{
		//�ҵ��������λ��
		//ȡ�����ֱ�ӿ飬Ҫ�����Ŀ¼��Ŀ��λ��
		fseek(fr, cur.i_direct_Block[posi], SEEK_SET);
		fread(dirlist, sizeof(dirlist), 1, fr);
		fflush(fr);
		//�������Ŀ¼��
		strcpy(dirlist[posj].itemName, name);	//Ŀ¼��
		//д��������¼ "." ".."���ֱ�ָ��ǰinode�ڵ��ַ���͸�inode�ڵ�
		int chiinoAddr = ialloc();	//���䵱ǰ�ڵ��ַ 
		if (error_!=0)
			return;
		dirlist[posj].inode_address = chiinoAddr; //������µ�Ŀ¼�����inode��ַ
		//��������Ŀ��inode
		Inode p;
		p.i_number = (chiinoAddr - kInodeStartAddress) / super_block_->s_inode_size;
		p.i_create_time = time(NULL);
		p.i_last_change_time = time(NULL);
		p.i_last_open_time = time(NULL);
		p.i_file_num = 2;	//�������ǰĿ¼,"."��".."
		//�������inode�Ĵ��̿飬�ڴ��̺���д��������¼ . �� ..
		int nextblockAddr = balloc();
		if (error_ != 0)
		{
			return;
		}
		DirItem dirlist2[32] = { 0 };	//��ʱĿ¼���б� - 2
		strcpy(dirlist2[0].itemName, ".");
		strcpy(dirlist2[1].itemName, "..");
		dirlist2[0].inode_address = chiinoAddr;	//��ǰĿ¼inode��ַ
		dirlist2[1].inode_address = previous_file;	//��Ŀ¼inode��ַ

		//д�뵽��ǰĿ¼�Ĵ��̿�
		fseek(fw, nextblockAddr, SEEK_SET);
		fwrite(dirlist2, sizeof(dirlist2), 1, fw);

		p.i_direct_Block[0] = nextblockAddr;
		int k;
		for (k = 1; k < 10; k++)
		{
			p.i_direct_Block[k] = 0;
		}
		p.i_size = super_block_->s_block_size;
		p.i_indirect_block_1 = 0;	//ûʹ��һ����ӿ�
		p.i_indirect_block_2 = 0;
		p.i_indirect_block_3 = 0;
		p.i_type = TYPE_DIR;

		//��inodeд�뵽�����inode��ַ
		fseek(fw, chiinoAddr, SEEK_SET);
		fwrite(&p, sizeof(Inode), 1, fw);

		//����ǰĿ¼�Ĵ��̿�д��
		fseek(fw, cur.i_direct_Block[posi], SEEK_SET);
		fwrite(dirlist, sizeof(dirlist), 1, fw);

		//д��inode
		cur.i_file_num++;
		fseek(fw, previous_file, SEEK_SET);
		fwrite(&cur, sizeof(Inode), 1, fw);
		fflush(fw);
		return;
	}


	for (i = 0; i < 10; i++)
		if (cur.i_direct_Block[i] == 0)
			break;
	if (i != 10)
	{
		if (error_ == 5)
			error_ = 0;
		int current_block_adress = balloc();
		if (error_ != 0)
			return;
		cur.i_direct_Block[i] = current_block_adress;
		DirItem dirlist3[32];

		int nextinoAddr = ialloc();	//���䵱ǰ�ڵ��ַ 
		if (error_ != 0)
			return;
		dirlist3[0].inode_address = nextinoAddr;
		strcpy(dirlist3[0].itemName, name);

		Inode p2;
		p2.i_number = (nextinoAddr - kInodeStartAddress) / super_block_->s_inode_size;
		p2.i_create_time = time(NULL);
		p2.i_last_change_time = time(NULL);
		p2.i_last_open_time = time(NULL);
		p2.i_file_num = 2;	//�������ǰĿ¼,"."��".."

		//�������inode�Ĵ��̿飬�ڴ��̺���д��������¼ . �� ..
		int nextblockAddr = balloc();
		p2.i_direct_Block[0] = nextblockAddr;
		if (error_ != 0)
		{
			return;
		}
		DirItem dirlist4[32] = { 0 };	//��ʱĿ¼���б� - 2
		strcpy(dirlist4[0].itemName, ".");
		strcpy(dirlist4[1].itemName, "..");
		dirlist4[0].inode_address = nextinoAddr;	//��ǰĿ¼inode��ַ
		dirlist4[1].inode_address = previous_file;	//��Ŀ¼inode��ַ

		//д�뵽��ǰĿ¼�Ĵ��̿�
		fseek(fw, nextblockAddr, SEEK_SET);
		fwrite(dirlist4, sizeof(dirlist4), 1, fw);

		int k;
		for (k = 1; k < 10; k++)
		{
			p2.i_direct_Block[k] = 0;
		}
		p2.i_size = super_block_->s_block_size;
		p2.i_indirect_block_1 = 0;	//ûʹ��һ����ӿ�
		p2.i_indirect_block_2 = 0;
		p2.i_indirect_block_3 = 0;
		p2.i_type = TYPE_DIR;

		//��inodeд�뵽�����inode��ַ
		fseek(fw, nextinoAddr, SEEK_SET);
		fwrite(&p2, sizeof(Inode), 1, fw);

		//����ǰĿ¼�Ĵ��̿�д��
		fseek(fw, cur.i_direct_Block[i], SEEK_SET);
		fwrite(dirlist3, sizeof(dirlist3), 1, fw);

		//д��inode
		cur.i_file_num++;
		fseek(fw, previous_file, SEEK_SET);
		fwrite(&cur, sizeof(Inode), 1, fw);
		fflush(fw);
		return;
	}*/
}

int FileSystem::DirectoryLookup(int previous_file, const char *name, bool type, int &posi, int &posj)
{
	DirItem dirlist[32];	//��ʱĿ¼�嵥
	//�������ַȡ��inode
	Inode cur;
	fseek(fr, previous_file, SEEK_SET);
	fread(&cur, sizeof(Inode), 1, fr);

	int i = 0;
	int cnt = cur.i_file_num ;	//Ŀ¼����
	posi = -1, posj = -1;
	while (i < 320) 
	{
		//320��Ŀ¼��֮�ڣ�����ֱ����ֱ�ӿ�����
		int dno = i / 32;	//�ڵڼ���ֱ�ӿ���,һ��ʮ��ֱ�ӿ�

		if (cur.i_direct_block[dno] == -1)
		{
			//û��ʹ��ֱ�ӿ�
			i += 32;
			continue;
		}
		//ȡ�����ֱ�ӿ飬Ҫ�����Ŀ¼��Ŀ��λ��
		fseek(fr, cur.i_direct_block[dno], SEEK_SET);
		fread(dirlist, sizeof(dirlist), 1, fr);
		fflush(fr);

		//����ô��̿��е�����Ŀ¼��
		int j;
		for (j = 0; j < 32; j++) 
		{
			if (strcmp(dirlist[j].itemName, name) == 0) 
			{
				if (dirlist[j].inode_address == 0) // �ҵ�һ�����м�¼������Ŀ¼���������λ��,��¼���λ��
				{
					if (posi == -1)
					{
						posi = dno;
						posj = j;
					}
					return 0;
				}
				else
				{
					Inode tmp;
					fseek(fr, dirlist[j].inode_address, SEEK_SET);
					fread(&tmp, sizeof(Inode), 1, fr);
					if (tmp.i_type == type)
					{
						if (posi == -1)
						{
							posi = dno;
							posj = j;
						}
						return  dirlist[j].inode_address;
					}
				}
			}
			i++;
		}
	}
	error_ = 5;
	return 0;
}

void FileSystem::Open()	//��װ�ļ�ϵͳ������������ļ��еĹؼ���Ϣ�糬������뵽�ڴ�
{
	root_directory_address_ = kInodeStartAddress;			//��һ��inode��ַ
	current_directory_address_ = root_directory_address_;
	current_directory_path_ = "/";

	//��д��������ļ�����ȡ�����飬��ȡinodeλͼ��blockλͼ����ȡ��Ŀ¼����ȡetcĿ¼����ȡ����ԱadminĿ¼����ȡ�û�xiaoĿ¼����ȡ�û�passwd�ļ���
	//��ȡ������
	fseek(fr, kSuperBlockStartAddress, SEEK_SET);//ת��ָ��
	fread(super_block_, sizeof(SuperBlock), 1, fr);//��ȡָ������
	//��ȡinodeλͼ
	fseek(fr, kInodeBitmapStartAddress, SEEK_SET);
	fread(inode_bitmap, sizeof(inode_bitmap), 1, fr);
	//��ȡblockλͼ
	fseek(fr, kBlockBitmapStartAddress, SEEK_SET);
	fread(block_bitmap, sizeof(block_bitmap), 1, fr);
	return;

}

void FileSystem::Parser()
{
	//
	//char inputcmd[100];
	//char absolutePath[100];
	//char *argv[64];
	//int count = 0, argc = 0;
	//printf("> "); // ÿ������ǰ�ı�־, �����ڡ�$ ~��
	//fgets(inputcmd, sizeof(inputcmd), stdin);
	//// ****************** close ���� ******************
	//if (strcmp(inputcmd, "close\n") == 0) 
	//{
	//	return;//�ر�
	//}
	////**********************������*******************
	//count = strlen(inputcmd) - 1;
	//for (int i = 0; i <= count; i++) // ��'\0'�滻' ','\n'��Ŀ����Ҫ��ָ������в���������ַ���
	//{ 
	//	if (inputcmd[i] == ' ' || inputcmd[i] == '\n')
	//		inputcmd[i] = '\0';
	//}
	//argv[argc] = &inputcmd[0];
	//argc++;
	//for (int i = 1; i <= count - 1; i++)  // ��ÿ���ַ�����λַ�浽argv
	//{
	//	if (inputcmd[i] == '\0' && inputcmd[i + 1] != '\0') 
	//	{
	//		argv[argc] = &inputcmd[i + 1];
	//		argc++;
	//	}
	//}
	//
	///**********************************
	//	 **********************************
	//	 **********************************
	//	 ********   Command APIs   ********
	//	 **********************************
	//	 **********************************
	//	 **********************************/

	//	 // ****************** att ���� *****************
	//if (strcmp(argv[0], "att") == 0) 
	//{
	//	// att ����, ������
	//}
	//// ****************** dr ���� , �൱�� ls ���� *****************
	//		// ****************** dr ���� , �൱�� ls ���� *****************
	//else if (strcmp(argv[0], "dr") == 0) 
	//{
	//	//strcpy(tempReturn, ".");
	//	//strcat(tempReturn, ":\n");
	//	//dir_ls(ls_return, PATH);
	//	//strcat(tempReturn, ls_return);

	//	if (argc == 1)
	//	{
	//		printf("%s", tempReturn);
	//	}
	//	else if (argc >= 2)
	//	{
	//		for (int i = 1; i <= argc - 1; i++)
	//		{
	//			if (strcmp(argv[i], ">") == 0 && (i + 1) <= argc - 1)
	//			{
	//				relative_to_absolute_path(argv[i + 1], absolutePath);
	//				printf("[write_file_by_path]path:%s buf=%s size=%lu\n", absolutePath, tempReturn, strlen(tempReturn));

	//				if (write_file_by_path(absolutePath, tempReturn, strlen(tempReturn)) >= 0)
	//				{
	//					//if(1){
	//					printf("%s finish\n", tempReturn);
	//					break;
	//				}
	//				else
	//				{
	//					printf("write file error.\n");
	//					break;
	//				}

	//			}
	//			else if (strcmp(argv[i], ">") == 0 && i == argc - 1)
	//			{
	//				printf("write file error\n");
	//				break;
	//			}
	//			else if (strcmp(argv[i], ">") != 0 && i == 1)
	//			{
	//				relative_to_absolute_path(argv[i], absolutePath);
	//				if (dir_change(absolutePath) < 0)
	//				{
	//					//if(0){
	//					strcpy(tempReturn, argv[i]);
	//					strcat(tempReturn, ":\n");
	//					strcat(tempReturn, "No such file or directory.\n");
	//				}
	//				else
	//				{
	//					strcpy(tempReturn, argv[i]);
	//					strcat(tempReturn, ":\n");
	//					dir_ls(ls_return, absolutePath);
	//					strcat(tempReturn, ls_return);
	//					//strcat(tempReturn,"dir1\ndate.txt\ncat.mp3\n");
	//				}
	//			}
	//			else if (strcmp(argv[i], ">") != 0) {
	//				relative_to_absolute_path(argv[i], absolutePath);
	//				if (dir_change(absolutePath) < 0) {
	//					//if(0){
	//					strcat(tempReturn, argv[i]);
	//					strcat(tempReturn, ":\n");
	//					strcat(tempReturn, "No such file or directory.\n");
	//				}
	//				else {
	//					strcat(tempReturn, argv[i]);
	//					strcat(tempReturn, ":\n");
	//					dir_ls(ls_return, absolutePath);
	//					strcat(tempReturn, ls_return);
	//					//strcat(tempReturn,"dir1\ndate.txt\ncat.mp3\n");
	//				}
	//			}
	//			if (i == argc - 1)
	//			{
	//				if (strcmp(tempReturn, "") != 0)
	//					printf("%s", tempReturn);
	//			}
	//		}
	//	}
	//}
}

void FileSystem::Cd(int previous_file, const char *path)
{
	std::string cd_path = path;
	if (cd_path[0] == '/')
	{
		int len = cd_path.find('/', 1), posi = -1, posj = -1,address=0;
		if (len != std::string::npos)
		{
			address=DirectoryLookup(root_directory_address_, cd_path.substr(1, len-1).c_str(), TYPE_DIR, posi, posj);
			if (error_!=0)
			{
				return;
			}
			Cd(address, cd_path.substr(cd_path.find('/', 1) + 1).c_str());
		}
		else
		{
			address=DirectoryLookup(root_directory_address_, cd_path.substr(1).c_str(), TYPE_DIR, posi, posj);
			if (error_ != 0)
			{
				return;
			}
			current_directory_path_ = "/" + cd_path.substr(1);
			current_directory_address_ = address;
		}
	}
	else if (cd_path[0] == '.')
	{
		if (cd_path.length() == 1)
		{
			return;
		}
		else if (cd_path[0] == '.')
		{
			Inode cur;
			fseek(fr, previous_file, SEEK_SET);
			fread(&cur, sizeof(Inode), 1, fr);
			DirItem dirlist[32];	//��ʱĿ¼�嵥
			fseek(fr, cur.i_direct_block[0], SEEK_SET);
			fread(dirlist, sizeof(dirlist), 1, fr);
			current_directory_address_ = dirlist[1].inode_address;
			current_directory_path_ = current_directory_path_.substr(1, current_directory_path_.find_last_of('/') - 1);
			return;
		}
	}
	else
	{
		int len = cd_path.find('/', 1), posi = -1, posj = -1, address = 0;
		if (len != std::string::npos)
		{
			address = DirectoryLookup(previous_file, cd_path.substr(1, len - 1).c_str(), TYPE_DIR, posi, posj);
			if (error_ != 0)
			{
				return;
			}
			Cd(address, cd_path.substr(cd_path.find('/', 1) + 1).c_str());
		}
		else
		{
			address = DirectoryLookup(previous_file, cd_path.substr(1).c_str(), TYPE_DIR, posi, posj);
			if (error_ != 0)
			{
				return;
			}
			current_directory_path_ = current_directory_path_+ "/" + cd_path.substr(1);
			current_directory_address_ = address;
			return;
		}
	}
}

void FileSystem::Move(int be_shared_folder,const char *be_shared_name, int shar_to_folder,int file_type)
{
	Inode be_shared_folder_inode;
	DirItem dirlist[32];	//��ʱĿ¼�嵥
	fseek(fr, be_shared_folder, SEEK_SET);
	fread(&be_shared_folder_inode, sizeof(Inode), 1, fr);

	int posi = -1, posj = -1;
	DirectoryLookup(be_shared_folder, be_shared_name, file_type, posi, posj);
	if (error_!=0)
		return;
	fseek(fr, be_shared_folder_inode.i_direct_block[posi], SEEK_SET);
	fread(dirlist, sizeof(dirlist), 1, fr);

	Inode file;
	fseek(fr, dirlist[posj].inode_address, SEEK_SET);
	fread(&file, sizeof(Inode), 1, fr);
	AddFileToFolder(shar_to_folder, dirlist[posj].inode_address, dirlist[posj].itemName, file.i_type);
	if (error_ != 0)
		return;
	if (file.i_type == TYPE_DIR)
	{
		DirItem dirlist2[32];	//��ʱĿ¼�嵥
		fseek(fr, file.i_direct_block[0], SEEK_SET);
		fread(dirlist2, sizeof(dirlist2), 1, fr);
		fflush(fr);
		dirlist2[1].inode_address = shar_to_folder;
		fseek(fw, file.i_direct_block[0], SEEK_SET);
		fwrite(dirlist2, sizeof(dirlist2), 1, fw);
		fflush(fw);
	}

	dirlist[posj].inode_address = 0;
	memset(dirlist[posj].itemName, 0, sizeof(dirlist[posj].itemName));
	fseek(fw, be_shared_folder_inode.i_direct_block[posi], SEEK_SET);
	fwrite(dirlist, sizeof(dirlist), 1, fw);
	be_shared_folder_inode.i_file_num--;
	fseek(fw, be_shared_folder, SEEK_SET);
	fwrite(&be_shared_folder_inode, sizeof(Inode), 1, fw);
	fflush(fw);
}

void FileSystem::AddFileToFolder(int folder_inode_adress, int file_inode_adress,const char* file_name,bool file_type)
{

	DirItem dirlist[32];	//��ʱĿ¼�嵥
	Inode folder_inode;
	int i;
	fseek(fr, folder_inode_adress, SEEK_SET);
	fread(&folder_inode, sizeof(Inode), 1, fr);
	if (folder_inode.i_file_num == 320)
	{
		error_ = 7;
		return;
	}
	if (folder_inode.i_type != TYPE_DIR)
	{
		error_ = 9;
		return;
	}
	int posi = -1, posj = -1;
	DirectoryLookup(folder_inode_adress, file_name, file_type, posi, posj);
	if (posi != -1 && posj != -1)//���ҵ�ͬ���ļ�
	{
		error_ = 8;
		return;
	}
	DirectoryLookup(folder_inode_adress, "", file_type, posi, posj);
	if (posi != -1 && posj != -1)		//�ҵ��������λ�ã�ȡ�����ֱ�ӿ飬Ҫ�����Ŀ¼��Ŀ��λ��
	{
		fseek(fr, folder_inode.i_direct_block[posi], SEEK_SET);
		fread(dirlist, sizeof(dirlist), 1, fr);
		fflush(fr);
		//�������Ŀ¼��
		strcpy(dirlist[posj].itemName, file_name);	//Ŀ¼��
		dirlist[posj].inode_address = file_inode_adress;

		//����ǰĿ¼�Ĵ��̿�д��
		fseek(fw, folder_inode.i_direct_block[posi], SEEK_SET);
		fwrite(dirlist, sizeof(dirlist), 1, fw);

		//д��inode
		folder_inode.i_file_num++;
		fseek(fw, folder_inode_adress, SEEK_SET);
		fwrite(&folder_inode, sizeof(Inode), 1, fw);
		fflush(fw);
		return;
	}
	
	for (i = 0; i < 10; i++)
		if (folder_inode.i_direct_block[i] == 0)
			break;
	if (i != 10)
	{
		if (error_ == 5)
			error_ = 0;
		int current_block_adress = balloc();
		if (error_ != 0)
			return;
		folder_inode.i_direct_block[i] = current_block_adress;
		DirItem dirlist2[32];

		dirlist2[0].inode_address = file_inode_adress;
		strcpy(dirlist2[0].itemName, file_name);

		//����ǰĿ¼�Ĵ��̿�д��
		fseek(fw, folder_inode.i_direct_block[i], SEEK_SET);
		fwrite(dirlist2, sizeof(dirlist2), 1, fw);

		//д��inode
		folder_inode.i_file_num++;
		fseek(fw, folder_inode_adress, SEEK_SET);
		fwrite(&folder_inode, sizeof(Inode), 1, fw);
		fflush(fw);
		return;
	}
}

void FileSystem::bfree(int addr)	//���̿��ͷź���
{
	//�ж�
	//�õ�ַ���Ǵ��̿����ʼ��ַ
	if ((addr - kBlockStartAddress) % super_block_->s_block_size != 0)
	{
		error_ = 10;
		return;
	}
	unsigned int bno = (addr - kBlockStartAddress) / super_block_->s_block_size;	//inode�ڵ��
	/*//�õ�ַ��δʹ�ã������ͷſռ�
	if (block_bitmap[bno] == 0) 
	{
		printf("��block�����̿飩��δʹ�ã��޷��ͷ�\n");
		return false;
	}*/

	//�����ͷ�
	//���㵱ǰջ��
	int top;	//ջ��ָ��
	if (super_block_->s_free_block_num == super_block_->s_block_num) //û�зǿ��еĴ��̿�
	{	
		error_ = 11;
		return;	//û�пɷ���Ŀ��п飬����-1
	}
	else 
	{	//����
		top = (super_block_->s_free_block_num - 1) % super_block_->s_blocks_per_group;

		//���block����
		char tmp[BLOCK_SIZE] = { 0 };
		fseek(fw, addr, SEEK_SET);
		fwrite(tmp, sizeof(tmp), 1, fw);

		if (top == super_block_->s_blocks_per_group - 1)//��ջ����
		{	
			//�ÿ��п���Ϊ�µĿ��п��ջ
			super_block_->s_free[0] = super_block_->s_free_addr;	//�µĿ��п��ջ��һ����ַָ��ɵĿ��п��ջָ��
			int i;
			for (i = 1; i < super_block_->s_blocks_per_group; i++) {
				super_block_->s_free[i] = -1;	//���ջԪ�ص�������ַ
			}
			fseek(fw, addr, SEEK_SET);
			fwrite(super_block_->s_free, sizeof(super_block_->s_free), 1, fw);	//����������̿飬512�ֽ�

		}
		else //ջ��δ��
		{	
			top++;	//ջ��ָ��+1
			super_block_->s_free[top] = addr;	//ջ���������Ҫ�ͷŵĵ�ַ����Ϊ�µĿ��п�
		}
	}


	//���³�����
	super_block_->s_free_block_num++;	//���п���+1
	fseek(fw, kSuperBlockStartAddress, SEEK_SET);
	fwrite(super_block_, sizeof(SuperBlock), 1, fw);

	//����blockλͼ
	block_bitmap[bno] = 0;
	fseek(fw, bno + kBlockBitmapStartAddress, SEEK_SET);	//(addr-Block_StartAddr)/BLOCK_SIZEΪ�ڼ������п�
	fwrite(&block_bitmap[bno], sizeof(bool), 1, fw);
	fflush(fw);

	return;
}

void FileSystem::ifree(int addr)	//�ͷ�i���������
{
	//�ж�
	if ((addr - kInodeStartAddress) % super_block_->s_inode_size != 0) 
	{
		error_ = 12;
		return;
	}
	unsigned short ino = (addr - kInodeStartAddress) / super_block_->s_inode_size;	//inode�ڵ��
	if (inode_bitmap[ino] == 0) 
	{
		error_ = 13;
		return;
	}

	//���inode����
	Inode tmp = { 0 };
	fseek(fw, addr, SEEK_SET);
	fwrite(&tmp, sizeof(tmp), 1, fw);

	//���³�����
	super_block_->s_free_inode_num++;
	//����inode��+1
	fseek(fw, kSuperBlockStartAddress, SEEK_SET);
	fwrite(super_block_, sizeof(SuperBlock), 1, fw);

	//����inodeλͼ
	inode_bitmap[ino] = 0;
	fseek(fw, kInodeBitmapStartAddress + ino, SEEK_SET);
	fwrite(&inode_bitmap[ino], sizeof(bool), 1, fw);
	fflush(fw);

	return;
}

void FileSystem::PutInRecycle(int previous_file, const char *file_name,bool file_type)
{
	int posi = -1, posj = -1;
	int file_address = DirectoryLookup(previous_file, file_name, file_type, posi, posj);
	if (error_ != 0)
		return;
	Inode file;
	fseek(fr, file_address, SEEK_SET);
	fread(&file, sizeof(Inode),1, fr);
	fflush(fr);
	file.i_pre_folder_adress = previous_file;
	AddFileToFolder(recycle_directory_address_, file_address, file_name, file.i_type);
	fseek(fw, file_address, SEEK_SET);
	fwrite(&file, sizeof(Inode), 1, fw);
	fflush(fw);
	return;
}

void FileSystem::RestoreFromRecycle(const char *file_name, bool file_type)
{
	int posi = -1, posj = -1;
	int file_address = DirectoryLookup(recycle_directory_address_, file_name, file_type, posi, posj);
	if (error_ != 0)
		return;
	Inode file;
	fseek(fr, file_address, SEEK_SET);
	fread(&file, sizeof(Inode), 1, fr);
	AddFileToFolder(recycle_directory_address_, file_address, file_name, file.i_type);
	file.i_pre_folder_adress = 0;
	return;
}

void FileSystem::EmptyRecycle(int inode_dress)	//��ջ���վ
{}

void FileSystem::win_cp_minfs(int parinoAddr, const char* name, const char* win_path) //win��minifs��������
{

	int chiinoAddr = ialloc();	//���䵱ǰ�ڵ��ַ 
	//��������Ŀ��inode
	Inode p;
	p.i_number = (chiinoAddr - kInodeStartAddress) / super_block_->s_inode_size;
	p.i_create_time = time(NULL);
	p.i_last_change_time = time(NULL);
	p.i_last_open_time = time(NULL);
	p.i_file_num = 1;//Ӳ����

	int len = 0;  //�ļ���С
	FILE *fp = fopen(win_path, "rb");
	if (fp == NULL)
	{
		error_ = 14;
		return;
	}
	fseek(fp, 0, SEEK_END);
	len = ftell(fp);
	if (len > super_block_->s_free_block_num*BLOCK_SIZE);
	{
		error_ = 15;
		return;
	}
	p.i_size = len; //��ȡ�ļ���С
	fclose(fp);
	if (len % 1024 != 0)
		len = ((len / 1024) + 1) * 1024;
	char *file_content = new char[len];
	fp = fopen(win_path, "rb");
	fread(file_content, len, 1, fp);   //file_content �������ļ����� len��ʾ�ļ�����
	int storage = 0;   //�Ѵ����С
	
	AddFileToFolder(parinoAddr, chiinoAddr, name, TYPE_FILE);
	if (error_==8)
	{
		error_ = 0;
		printf("�ļ��Ѵ���\n");
		printf("����/������(y/n)\n");
		char ju;
		scanf("%c", &ju);
		if (ju == 'y')
		{
			//ɾ��ԭinode
		}
		else if (ju == 'n')
		{
			printf("�������µ��ļ���:");
			char *a = new char[80];
			scanf("%s", a);
			win_cp_minfs(parinoAddr, a, win_path);
			delete a;
			return;
		}
	}

	if (error_ != 0)
		return;
	for (int i = 0; i < 10; i++)   //����ֱ�ӿ�
	{
		p.i_direct_block[i] = balloc();
		if (error_ != 0)
			return;
		fseek(fw, p.i_direct_block[i], SEEK_SET);
		fwrite(file_content + storage, 1024, 1, fw);
		storage += 1024;
		if (storage >= len)
			break;
	}

	if (storage >= len)
	{
		delete file_content;
		return;
	}

	p.i_indirect_block_1 = balloc();    //����һ����ӿ�
	if (error_ != 0)
		return;
	int *tmp = new int[256];
	for (int i = 0; i < 256; i++)
	{
		tmp[i] = balloc();
		if (error_ != 0)
			return;
		fseek(fw, tmp[i], SEEK_SET);
		fwrite(file_content + storage, 1024, 1, fw);
		storage += 1024;
		if (storage >= len)
			break;
	}
	fseek(fw, p.i_indirect_block_1, SEEK_SET);
	fwrite(tmp, 1024, 1, fw);
	delete tmp;

	if (storage >= len)
	{
		delete file_content;
		return;
	}

	p.i_indirect_block_2 = balloc(); //���������ӿ�
	if (error_ != 0)
		return;
	tmp = new int[256];
	for (int i = 0; i < 256; i++)
	{
		int* tmp_1 = new int[256];
		for (int j = 0; j < 256; j++)
		{
			tmp_1[j] = balloc();
			if (error_ != 0)
				return;
			fseek(fw, tmp_1[j], SEEK_SET);
			fwrite(file_content + storage, 1024, 1, fw);
			storage += 1024;
			if (storage >= len)
				break;
		}
		fseek(fw, tmp[i], SEEK_SET);
		fwrite(tmp_1, 1024, 1, fw);
		delete tmp_1;
		if (storage >= len)
			break;
	}
	fseek(fw, p.i_indirect_block_2, SEEK_SET);
	fwrite(tmp, 1024, 1, fw);
	delete tmp;

	if (storage >= len)
	{
		delete file_content;
		return;
	}

	p.i_indirect_block_3 = balloc();//����������ӿ�
	if (error_ != 0)
		return;
	tmp = new int[256];
	for (int i = 0; i < 256; i++)
	{
		tmp[i] = balloc();
		if (error_ != 0)
			return;
		int* tmp_1 = new int[256];
		for (int j = 0; j < 256; j++)
		{
			tmp_1[j] = balloc();
			if (error_ != 0)
				return;
			int* tmp_2 = new int[256];
			for (int p = 0; p < 256; p++)
			{
				tmp_2[p] = balloc();
				if (error_ != 0)
					return;
				fseek(fw, tmp_2[p], SEEK_SET);
				fwrite(file_content + storage, 1024, 1, fw);
				storage += 1024;
				if (storage >= len)
					break;
			}
			fseek(fw, tmp_1[j], SEEK_SET);
			fwrite(tmp_2, 1024, 1, fw);
			delete tmp_2;
			if (storage >= len)
				break;
		}
		fseek(fw, tmp[i], SEEK_SET);
		fwrite(tmp_1, 1024, 1, fw);
		delete tmp_1;
		if (storage >= len)
			break;
	}
	fseek(fw, p.i_indirect_block_3, SEEK_SET);
	fwrite(tmp, 1024, 1, fw);
	delete tmp;

	delete file_content;

	p.i_type = TYPE_FILE;
	fseek(fw, chiinoAddr, SEEK_SET);
	fwrite(&p, sizeof(Inode), 1, fw);
	return;

}

void FileSystem::minifs_cp_win(int parinoAddr, const char* name, const char* win_path)//minifs��win����
{
//	DirItem dirlist[32]; //��ʱĿ¼�嵥
	Inode cur;
	fseek(fr, parinoAddr, SEEK_SET);
	fread(&cur, sizeof(Inode), 1, fr); //ȡ��inode
	int len = cur.i_size;
	if (cur.i_size % 1024 != 0)
	{
		len = ((cur.i_size / 1024) + 1) * 1024;
	}
	char *file_content = new char[len];  //�����ռ�
	char *mov_ptr = file_content;      //�ƶ�ָ��
	for (int i = 0; i < 10; i++)    //��ȡֱ�ӿ�
	{
		if (cur.i_direct_block[i] == 0)
			break;
		fseek(fr, cur.i_direct_block[i], SEEK_SET);
		fread(mov_ptr, 1024, 1, fr);
		mov_ptr += 1024;
	}

	if (cur.i_indirect_block_1 != 0)   //��ȡһ����ӿ�
	{
		int* tmp = new int[256];
		fseek(fr, cur.i_indirect_block_1, SEEK_SET);
		fread(tmp, 1024, 1, fr);
		for (int i = 0; i < 256; i++)
		{
			if (tmp[i] == 0)
				break;
			fseek(fr, tmp[i], SEEK_SET);
			fread(mov_ptr, 1024, 1, fr);
			mov_ptr += 1024;
		}
		delete tmp;
	}
	if (cur.i_indirect_block_2 != 0)  //��ȡ������ӿ�
	{
		int* tmp = new int[256];
		fseek(fr, cur.i_indirect_block_2, SEEK_SET);
		fread(tmp, 1024, 1, fr);
		for (int i = 0; i < 256; i++)
		{
			if (tmp[i] == 0)
				break;
			int *tmp_1 = new int[256];
			fseek(fr, tmp[i], SEEK_SET);
			fread(tmp_1, 1024, 1, fr);
			for (int j = 0; j < 256; j++)
			{
				if (tmp_1[j] == 0)
					break;
				fseek(fr, tmp_1[j], SEEK_SET);
				fread(mov_ptr, 1024, 1, fr);
				mov_ptr += 1024;
			}
			delete tmp_1;
		}
		delete tmp;
	}
	if (cur.i_indirect_block_3 != 0)  //��ȡ������ӿ�
	{
		int* tmp = new int[256];
		fseek(fr, cur.i_indirect_block_3, SEEK_SET);
		fread(tmp, 1024, 1, fr);
		for (int i = 0; i < 256; i++)
		{
			if (tmp[i] == 0)
				break;
			int* tmp_1 = new int[256];
			fseek(fr, tmp[i], SEEK_SET);
			fread(tmp_1, 1024, 1, fr);
			for (int j = 0; j < 256; j++)
			{
				if (tmp_1[j] == 0)
					break;
				int* tmp_2 = new int[256];
				fseek(fr, tmp_1[j], SEEK_SET);
				fread(tmp_2, 1024, 1, fr);
				for (int p = 0; p < 256; p++)
				{
					if (tmp_2[p] == 0)
						break;
					fseek(fr, tmp_2[p], SEEK_SET);
					fread(mov_ptr, 1024, 1, fr);
					mov_ptr += 1024;
				}
				delete tmp_2;
			}
			delete tmp_1;
		}
		delete tmp;
	}
	FILE* win_file = fopen(win_path, "wb");
	fwrite(file_content, len, 1, win_file);
	delete file_content;
	return;
}

/*bool minifs_cp_minifs(int parinoAddr, int parinoAddr2, const char name[])//minifs��minifs����  ԭ Ŀ�� Ŀ��
{
	if (strlen(name) >= MAX_NAME_SIZE)
	{
		printf("��������ļ�������\n");
		return false;
	}
	DirItem dirlist[32]; //��ʱĿ¼�嵥
	Inode cur_2;
	fseek(fr, parinoAddr2, SEEK_SET);
	fread(&cur_2, sizeof(Inode), 1, fr); //ȡ��inode
	int i = 0;
	int posi = -1, posj = -1;
	int dno;
	int cnt = cur_2.i_cnt + 1;
	while (i < 320)   //320��Ŀ¼��
	{
		dno = i / 32; //�ڼ���ֱ�ӿ���
		if (cur_2.i_dirBlock[dno] == -1)
		{
			i += 32;
			continue;
		}
	}
	fseek(fr, cur_2.i_dirBlock[dno], SEEK_SET);
	fread(dirlist, sizeof(dirlist), 1, fr);
	fflush(fr);
	int j;
	for (j = 0; j < 16; j++)  //���Ҹÿ�����Ŀ¼��
	{
		if (posi == -1 && strcmp(dirlist[j].itemName, "") == 0) //�ҿ��м�¼�����ļ����������λ��
		{
			posi = dno;
			posj = j;
		}
		else if (strcmp(dirlist[j].itemName, name) == 0)  //�������ж��Ƿ�Ϊ�ļ�
		{
			Inode cur2;
			fseek(fr, dirlist[j].inodeAddr, SEEK_SET);
			fread(&cur2, sizeof(Inode), 1, fr);
			if (((cur2.i_mode >> 9) & 1) == 0)    //������Ϊ�ļ������ܴ���
			{
				printf("�ļ��Ѵ���\n");
				printf("����/������(y/n)\n");
				char ju;
				scanf("%c", &ju);
				if (ju == 'y')
				{
					posi = dno;
					posj = j;
					break;
				}
				else if (ju == 'n')
				{
					printf("�������µ��ļ���:");
					char *a = new char[80];
					scanf("%s", a);
					if (minifs_cp_minifs(parinoAddr, parinoAddr2, a))
					{
						delete a;
						return true;
					}
				}
				return false;
			}
		}
		i++;
	}
	if (posi != -1)   //�ҵ�Ŀ¼��
	{
		fseek(fr, cur_2.i_dirBlock[posi], SEEK_SET);
		fread(dirlist, sizeof(dirlist), 1, fr);       //ȡ�����̿�
		fflush(fr);
		strcpy(dirlist[posj].itemName, name);
		int chiinoAddr = ialloc();      //����inode�ڵ�
		if (chiinoAddr == -1)
		{
			printf("����ʧ��\n");
			return false;
		}
		dirlist[posj].inodeAddr = chiinoAddr;  //����inode�ڵ�
		Inode p;
		p.i_ino = (chiinoAddr - Inode_StartAddr) / superblock->s_INODE_SIZE;
		p.i_atime = time(NULL);
		p.i_ctime = time(NULL);
		p.i_mtime = time(NULL);
		p.i_cnt = 1;


		Inode cur;
		fseek(fr, parinoAddr, SEEK_SET);
		fread(&cur, sizeof(Inode), 1, fr); //ȡ��inode
		int len = cur.i_size;
		if (cur.i_size % 1024 != 0)
		{
			len = ((cur.i_size / 1024) + 1) * 1024;
		}
		p.i_size = len;
		char *file_content = new char[len];  //�����ռ�
		char *mov_ptr = file_content;      //�ƶ�ָ��
		for (int i = 0; i < 10; i++)    //��ȡֱ�ӿ�
		{
			if (cur.i_dirBlock[i] == 0)
				break;
			fseek(fr, cur.i_dirBlock[i], SEEK_SET);
			fread(mov_ptr, 1024, 1, fr);
			mov_ptr += 1024;
		}

		if (cur.i_indirBlock_1 != 0)   //��ȡһ����ӿ�
		{
			int* tmp = new int[256];
			fseek(fr, cur.i_indirBlock_1, SEEK_SET);
			fread(tmp, 1024, 1, fr);
			for (int i = 0; i < 256; i++)
			{
				if (tmp[i] == 0)
					break;
				fseek(fr, tmp[i], SEEK_SET);
				fread(mov_ptr, 1024, 1, fr);
				mov_ptr += 1024;
			}
			delete tmp;
		}
		if (cur.i_indirBlock_2 != 0)  //��ȡ������ӿ�
		{
			int* tmp = new int[256];
			fseek(fr, cur.i_indirBlock_2, SEEK_SET);
			fread(tmp, 1024, 1, fr);
			for (int i = 0; i < 256; i++)
			{
				if (tmp[i] == 0)
					break;
				int *tmp_1 = new int[256];
				fseek(fr, tmp[i], SEEK_SET);
				fread(tmp_1, 1024, 1, fr);
				for (int j = 0; j < 256; j++)
				{
					if (tmp_1[j] == 0)
						break;
					fseek(fr, tmp_1[j], SEEK_SET);
					fread(mov_ptr, 1024, 1, fr);
					mov_ptr += 1024;
				}
				delete tmp_1;
			}
			delete tmp;
		}
		if (cur.i_indirBlock_3 != 0)  //��ȡ������ӿ�
		{
			int* tmp = new int[256];
			fseek(fr, cur.i_indirBlock_3, SEEK_SET);
			fread(tmp, 1024, 1, fr);
			for (int i = 0; i < 256; i++)
			{
				if (tmp[i] == 0)
					break;
				int* tmp_1 = new int[256];
				fseek(fr, tmp[i], SEEK_SET);
				fread(tmp_1, 1024, 1, fr);
				for (int j = 0; j < 256; j++)
				{
					if (tmp_1[j] == 0)
						break;
					int* tmp_2 = new int[256];
					fseek(fr, tmp_1[j], SEEK_SET);
					fread(tmp_2, 1024, 1, fr);
					for (int p = 0; p < 256; p++)
					{
						if (tmp_2[p] == 0)
							break;
						fseek(fr, tmp_2[p], SEEK_SET);
						fread(mov_ptr, 1024, 1, fr);
						mov_ptr += 1024;
					}
					delete tmp_2;
				}
				delete tmp_1;
			}
			delete tmp;
		}

		int storage = 0;   //�Ѵ����С

		for (int i = 0; i < 10; i++)   //����ֱ�ӿ�
		{
			p.i_dirBlock[i] = balloc();
			fseek(fw, p.i_dirBlock[i], SEEK_SET);
			fread(file_content + storage, 1024, 1, fw);
			storage += 1024;
			if (storage >= len)
				break;
		}

		if (storage >= len)
		{
			delete file_content;
			return true;
		}

		p.i_indirBlock_1 = balloc();    //����һ����ӿ�
		int *tmp = new int[256];
		for (int i = 0; i < 256; i++)
		{
			tmp[i] = balloc();
			fseek(fw, tmp[i], SEEK_SET);
			fwrite(file_content + storage, 1024, 1, fw);
			storage += 1024;
			if (storage >= len)
				break;
		}
		fseek(fw, p.i_indirBlock_1, SEEK_SET);
		fwrite(tmp, 1024, 1, fw);
		delete tmp;

		if (storage >= len)
		{
			delete file_content;
			return true;
		}

		p.i_indirBlock_2 = balloc(); //���������ӿ�
		int* tmp = new int[256];
		for (int i = 0; i < 256; i++)
		{
			int* tmp_1 = new int[256];
			for (int j = 0; j < 256; j++)
			{
				tmp_1[j] = balloc();
				fseek(fw, tmp_1[j], SEEK_SET);
				fwrite(file_content + storage, 1024, 1, fw);
				storage += 1024;
				if (storage >= len)
					break;
			}
			fseek(fw, tmp[i], SEEK_SET);
			fwrite(tmp_1, 1024, 1, fw);
			delete tmp_1;
			if (storage >= len)
				break;
		}
		fseek(fw, p.i_indirBlock_1, SEEK_SET);
		fwrite(tmp, 1024, 1, fw);
		delete tmp;

		if (storage >= len)
		{
			delete file_content;
			return true;
		}

		p.i_indirBlock_3 = balloc();//����������ӿ�
		int* tmp = new int[256];
		for (int i = 0; i < 256; i++)
		{
			int* tmp_1 = new int[256];
			for (int j = 0; j < 256; j++)
			{
				int* tmp_2 = new int[256];
				for (int p = 0; p < 256; p++)
				{
					tmp_2[p] = balloc();
					fseek(fw, tmp_2[p], SEEK_SET);
					fwrite(file_content + storage, 1024, 1, fw);
					storage += 1024;
					if (storage >= len)
						break;
				}
				fseek(fw, tmp_1[j], SEEK_SET);
				fwrite(tmp_2, 1024, 1, fw);
				delete tmp_2;
				if (storage >= len)
					break;
			}
			fseek(fw, tmp[i], SEEK_SET);
			fwrite(tmp_1, 1024, 1, fw);
			delete tmp_1;
			if (storage >= len)
				break;
		}
		fseek(fw, p.i_indirBlock_3, SEEK_SET);
		fwrite(tmp, 1024, 1, fw);
		delete tmp;

		delete file_content;
		if (storage < len)
		{
			printf("�ļ�����\n");
		}
		p.i_mode = 0;
		fseek(fw, chiinoAddr, SEEK_SET);
		fwrite(&p, sizeof(Inode), 1, fw);

		fseek(fw, cur.i_dirBlock[posi], SEEK_SET);  //д��Ŀ¼���̿�
		fwrite(dirlist, sizeof(dirlist), 1, fw);

		cur.i_cnt++;
		fseek(fw, parinoAddr, SEEK_SET);
		fwrite(&cur, sizeof(Inode), 1, fw);
		fflush(fw);
		return true;




	}
	return false;
}*/

void FileSystem::more(int parinoAddr)//��ҳ��ʾ
{
	Inode cur;
	fseek(fr, parinoAddr, SEEK_SET);
	fread(&cur, sizeof(Inode), 1, fr); //ȡ��inode
	int len = cur.i_size;
	if (cur.i_size % 1024 != 0)
	{
		len = ((cur.i_size / 1024) + 1) * 1024;
	}
	char *file_content = new char[len];  //�����ռ�
	char *mov_ptr = file_content;      //�ƶ�ָ��
	for (int i = 0; i < 10; i++)    //��ȡֱ�ӿ�
	{
		if (cur.i_direct_block[i] == 0)
			break;
		fseek(fr, cur.i_direct_block[i], SEEK_SET);
		fread(mov_ptr, 1024, 1, fr);
		mov_ptr += 1024;
	}

	if (cur.i_indirect_block_1 != 0)   //��ȡһ����ӿ�
	{
		int* tmp = new int[256];
		fseek(fr, cur.i_indirect_block_1, SEEK_SET);
		fread(tmp, 1024, 1, fr);
		for (int i = 0; i < 256; i++)
		{
			if (tmp[i] == 0)
				break;
			fseek(fr, tmp[i], SEEK_SET);
			fread(mov_ptr, 1024, 1, fr);
			mov_ptr += 1024;
		}
		delete tmp;
	}
	if (cur.i_indirect_block_2 != 0)  //��ȡ������ӿ�
	{
		int* tmp = new int[256];
		fseek(fr, cur.i_indirect_block_2, SEEK_SET);
		fread(tmp, 1024, 1, fr);
		for (int i = 0; i < 256; i++)
		{
			if (tmp[i] == 0)
				break;
			int *tmp_1 = new int[256];
			fseek(fr, tmp[i], SEEK_SET);
			fread(tmp_1, 1024, 1, fr);
			for (int j = 0; j < 256; j++)
			{
				if (tmp_1[j] == 0)
					break;
				fseek(fr, tmp_1[j], SEEK_SET);
				fread(mov_ptr, 1024, 1, fr);
				mov_ptr += 1024;
			}
			delete tmp_1;
		}
		delete tmp;
	}
	if (cur.i_indirect_block_3 != 0)  //��ȡ������ӿ�
	{
		int* tmp = new int[256];
		fseek(fr, cur.i_indirect_block_3, SEEK_SET);
		fread(tmp, 1024, 1, fr);
		for (int i = 0; i < 256; i++)
		{
			if (tmp[i] == 0)
				break;
			int* tmp_1 = new int[256];
			fseek(fr, tmp[i], SEEK_SET);
			fread(tmp_1, 1024, 1, fr);
			for (int j = 0; j < 256; j++)
			{
				if (tmp_1[j] == 0)
					break;
				int* tmp_2 = new int[256];
				fseek(fr, tmp_1[j], SEEK_SET);
				fread(tmp_2, 1024, 1, fr);
				for (int p = 0; p < 256; p++)
				{
					if (tmp_2[p] == 0)
						break;
					fseek(fr, tmp_2[p], SEEK_SET);
					fread(mov_ptr, 1024, 1, fr);
					mov_ptr += 1024;
				}
				delete tmp_2;
			}
			delete tmp_1;
		}
		delete tmp;
	}
	int h, count;
	char a;
	for (h = 0; h < strlen(file_content); h++)
	{
		if (file_content[h] == '\n')
			count++;
		if (count % 24 == 0)
		{
			//scanf("%c", &a);
			system("pause");
			system("cls");
		}
		printf("%c",file_content[h]);
	}
	delete file_content;
}

void FileSystem::att(int parinoAddr,const char *file)//��ʾ�ռ��ļ�����
{/*
	std::string fi = file;
	printf("����");
	printf("\tInode");
	printf("\t�ļ�");
	printf("\t�ļ���");
	printf("\t�ļ���С");
	printf("\t����ʱ��");
	printf("\t�޸�ʱ��");
	printf("\t��ʱ��");
	printf("\n");
	std::string pri;
	for (int i = 0; i < fi.length(); i++)
	{
		if (fi[0] == ' ')
		{
			int posi = -1, posj = -1;
			DirectoryLookup(parinoAddr, file, TYPE_DIR, posi, posj);
			pri = "";
		}
		else
		{
			pri[pri.length() - 1] = fi[i];
		}
	}
	return;
	*/
}

void FileSystem::type_txt(int parinoAddr)  //��ʾ��ǰ�ļ����µ��ı��ļ�
{
	Inode cur;
	fseek(fr, parinoAddr, SEEK_SET);
	fread(&cur, sizeof(Inode), 1, fr); //ȡ��inode
	int len = cur.i_size;
	if (cur.i_size % 1024 != 0)
	{
		len = ((cur.i_size / 1024) + 1) * 1024;
	}
	char *file_content = new char[len];  //�����ռ�
	char *mov_ptr = file_content;      //�ƶ�ָ��
	for (int i = 0; i < 10; i++)    //��ȡֱ�ӿ�
	{
		if (cur.i_direct_block[i] == 0)
			break;
		fseek(fr, cur.i_direct_block[i], SEEK_SET);
		fread(mov_ptr, 1024, 1, fr);
		mov_ptr += 1024;
	}

	if (cur.i_indirect_block_1 != 0)   //��ȡһ����ӿ�
	{
		int* tmp = new int[256];
		fseek(fr, cur.i_indirect_block_1, SEEK_SET);
		fread(tmp, 1024, 1, fr);
		for (int i = 0; i < 256; i++)
		{
			if (tmp[i] == 0)
				break;
			fseek(fr, tmp[i], SEEK_SET);
			fread(mov_ptr, 1024, 1, fr);
			mov_ptr += 1024;
		}
		delete tmp;
	}
	if (cur.i_indirect_block_2 != 0)  //��ȡ������ӿ�
	{
		int* tmp = new int[256];
		fseek(fr, cur.i_indirect_block_2, SEEK_SET);
		fread(tmp, 1024, 1, fr);
		for (int i = 0; i < 256; i++)
		{
			if (tmp[i] == 0)
				break;
			int *tmp_1 = new int[256];
			fseek(fr, tmp[i], SEEK_SET);
			fread(tmp_1, 1024, 1, fr);
			for (int j = 0; j < 256; j++)
			{
				if (tmp_1[j] == 0)
					break;
				fseek(fr, tmp_1[j], SEEK_SET);
				fread(mov_ptr, 1024, 1, fr);
				mov_ptr += 1024;
			}
			delete tmp_1;
		}
		delete tmp;
	}
	if (cur.i_indirect_block_3 != 0)  //��ȡ������ӿ�
	{
		int* tmp = new int[256];
		fseek(fr, cur.i_indirect_block_3, SEEK_SET);
		fread(tmp, 1024, 1, fr);
		for (int i = 0; i < 256; i++)
		{
			if (tmp[i] == 0)
				break;
			int* tmp_1 = new int[256];
			fseek(fr, tmp[i], SEEK_SET);
			fread(tmp_1, 1024, 1, fr);
			for (int j = 0; j < 256; j++)
			{
				if (tmp_1[j] == 0)
					break;
				int* tmp_2 = new int[256];
				fseek(fr, tmp_1[j], SEEK_SET);
				fread(tmp_2, 1024, 1, fr);
				for (int p = 0; p < 256; p++)
				{
					if (tmp_2[p] == 0)
						break;
					fseek(fr, tmp_2[p], SEEK_SET);
					fread(mov_ptr, 1024, 1, fr);
					mov_ptr += 1024;
				}
				delete tmp_2;
			}
			delete tmp_1;
		}
		delete tmp;
	}
	printf("%s", file_content);
	delete file_content;
	return;
}

void FileSystem::_Find(const char *a, int parinoAddr, const char *b) 
{
	Inode file;//��ǰ�ļ�
	std::string s = b;//������ǰĿ¼�ַ���
	std::string s1;
	FILE *fp = fopen(FILESYSNAME, "rb");
	fseek(fp, parinoAddr, SEEK_SET);
	fread(&file, sizeof(Inode), 1, fp);
	int i = 0;
	while (i < 320) {//����ʮ�����̿�
		DirItem dirlist[32] = { 0 };//��Ӧÿ�����̿��е�32��Ŀ¼��
		if (file.i_direct_block[i / 32] == -1) {
			i += 32;
			continue;//δʹ��������
		}
		int parblockAddr = file.i_direct_block[i / 32];//��i/32�Ŵ��̿�
		fseek(fp, parblockAddr, SEEK_SET);
		fread(&dirlist, sizeof(dirlist), 1, fp);//���ô��̿��е����ݶ���dirlist�У��Ӷ����ԶԸô��̿��п��ܴ��ڵ�Ŀ¼����в���
		int j;
		for (j = 0; j < 32; j++) {//��0��31����Ӧÿ��Ŀ¼��
			Inode tmp;//��ǰ��������Ŀ¼������Ӧ��inode
			fseek(fp, dirlist[j].inode_address, SEEK_SET);//ȷ����j��Ŀ¼���inodeλ��
			fread(&tmp, sizeof(Inode), 1, fp);//��ȡ��inode
			if (tmp.i_type == TYPE_DIR) // ���Ϊ�ļ�
			{
				if (strstr(dirlist[j].itemName, a) != NULL) {
					s1 = s;
					s1 = s1 + dirlist[j].itemName;
					printf("%s", s1.c_str());//path_output(parinoAddr, dirlist[j].itemName);//������ͬ�����
				}
			}
			else {//���Ϊ�ļ���
				if (strstr(dirlist[j].itemName, a) != NULL) {
					s1 = s;
					s1 = s1 + dirlist[j].itemName + '/';
					printf("%s", s1.c_str());//path_output(parinoAddr,dirlist[j].itemName);//������ͬ�����
				}
				_Find(a, dirlist[j].inode_address, s1.c_str());//����Ѱ��
			}
			i++;
		}
	}
}

void FileSystem::find(const char *a)//Ѱ���ļ�
{
	std::string b = "/";
	_Find(a, root_directory_address_, b.c_str());
}

