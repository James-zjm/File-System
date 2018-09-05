#include"pch.h"
#include"try.h"

bool Format()	//��ʽ��һ����������ļ�
{
	int i, j;

	//��ʼ��������
	superblock->s_INODE_NUM = INODE_NUM;
	superblock->s_BLOCK_NUM = BLOCK_NUM;
	superblock->s_SUPERBLOCK_SIZE = sizeof(SuperBlock);//560
	superblock->s_INODE_SIZE = INODE_SIZE;
	superblock->s_BLOCK_SIZE = BLOCK_SIZE;
	superblock->s_free_INODE_NUM = INODE_NUM;
	superblock->s_free_BLOCK_NUM = BLOCK_NUM;
	superblock->s_blocks_per_group = BLOCKS_PER_GROUP;
	superblock->s_free_addr = Block_StartAddr;	//���п��ջָ��Ϊ��һ��block
	superblock->s_Superblock_StartAddr = Superblock_StartAddr;
	superblock->s_BlockBitmap_StartAddr = BlockBitmap_StartAddr;
	superblock->s_InodeBitmap_StartAddr = InodeBitmap_StartAddr;
	superblock->s_Block_StartAddr = Block_StartAddr;
	superblock->s_Inode_StartAddr = Inode_StartAddr;
	//���п��ջ�ں��渳ֵ

	//��ʼ��inodeλͼ
	memset(inode_bitmap, 0, sizeof(inode_bitmap));
	fseek(fw, InodeBitmap_StartAddr, SEEK_SET);
	fwrite(inode_bitmap, sizeof(inode_bitmap), 1, fw);

	//��ʼ��blockλͼ
	memset(block_bitmap, 0, sizeof(block_bitmap));
	fseek(fw, BlockBitmap_StartAddr, SEEK_SET);
	fwrite(block_bitmap, sizeof(block_bitmap), 1, fw);

	//��ʼ�����̿��������ݳ������ӷ���֯	
	for (i = BLOCK_NUM / BLOCKS_PER_GROUP - 1; i >= 0; i--) 
	{	//һ��INODE_NUM/BLOCKS_PER_GROUP 8192 �飬һ��FREESTACKNUM��128�������̿� ����һ�����̿���Ϊ����
		if (i == BLOCK_NUM / BLOCKS_PER_GROUP - 1)
			superblock->s_free[0] = -1;	//û����һ�����п���
		else
			superblock->s_free[0] = Block_StartAddr + (i + 1)*BLOCKS_PER_GROUP*BLOCK_SIZE;	//ָ����һ�����п�
		for (j = 1; j < BLOCKS_PER_GROUP; j++) 
		{
			superblock->s_free[j] = Block_StartAddr + (i*BLOCKS_PER_GROUP + j)*BLOCK_SIZE;
		}
		fseek(fw, Block_StartAddr + i * BLOCKS_PER_GROUP*BLOCK_SIZE, SEEK_SET);
		fwrite(superblock->s_free, sizeof(superblock->s_free), 1, fw);	//����������̿飬512�ֽ�
	}
	//������д�뵽��������ļ�
	fseek(fw, Superblock_StartAddr, SEEK_SET);
	fwrite(superblock, sizeof(SuperBlock), 1, fw);

	fflush(fw);//ǿ�Ƚ��������ڵ�����д�ز���stream ָ�����ļ���

	//��ȡinodeλͼ
	fseek(fr, InodeBitmap_StartAddr, SEEK_SET);
	fread(inode_bitmap, sizeof(inode_bitmap), 1, fr);

	//��ȡblockλͼ
	fseek(fr, BlockBitmap_StartAddr, SEEK_SET);
	fread(block_bitmap, sizeof(block_bitmap), 1, fr);

	fflush(fr);

	//������Ŀ¼ "/"
	Inode cur;

	//����inode
	int inoAddr = ialloc();

	//�����inode������̿�
	int blockAddr = balloc();

	//��������̿������һ����Ŀ "."
	DirItem dirlist[32] = { 0 };
	strcpy(dirlist[0].itemName, ".");
	dirlist[0].inodeAddr = inoAddr;

	//д�ش��̿�
	fseek(fw, blockAddr, SEEK_SET);
	fwrite(dirlist, sizeof(dirlist), 1, fw);

	//��inode��ֵ
	cur.i_ino = 0;
	cur.i_atime = time(NULL);
	cur.i_ctime = time(NULL);
	cur.i_mtime = time(NULL);
	cur.i_cnt = 1;	//һ�����ǰĿ¼,"."��Ŀ¼
	cur.i_dirBlock[0] = blockAddr;
	for (i = 1; i < 10; i++) {
		cur.i_dirBlock[i] = -1;
	}
	cur.i_size = superblock->s_BLOCK_SIZE;
	cur.i_indirBlock_1 = -1;	//ûʹ��һ����ӿ�
	cur.i_indirBlock_2 = -1;
	cur.i_indirBlock_3 = -1;
	cur.i_mode = MODE_DIR ;

	//д��inode
	fseek(fw, inoAddr, SEEK_SET);
	fwrite(&cur, sizeof(Inode), 1, fw);
	fflush(fw);

	return true;
}

int ialloc()	//����i�ڵ�������������inode��ַ
{
	//��inodeλͼ��˳����ҿ��е�inode���ҵ��򷵻�inode��ַ������������
	if (superblock->s_free_INODE_NUM == 0) 
	{
		printf("û�п���inode���Է���\n");
		return -1;
	}
	else 
	{

		//˳����ҿ��е�inode
		int i;
		for (i = 0; i < superblock->s_INODE_NUM; i++) 
		{
			if (inode_bitmap[i] == 0)	//�ҵ�����inode
				break;
		}


		//���³�����
		superblock->s_free_INODE_NUM--;	//����inode��-1
		fseek(fw, Superblock_StartAddr, SEEK_SET);
		fwrite(superblock, sizeof(SuperBlock), 1, fw);

		//����inodeλͼ
		inode_bitmap[i] = 1;
		fseek(fw, InodeBitmap_StartAddr + i, SEEK_SET);
		fwrite(&inode_bitmap[i], sizeof(bool), 1, fw);
		fflush(fw);

		return Inode_StartAddr + i * superblock->s_INODE_SIZE;
	}
}

int balloc()	//���̿���亯��
{
	//ʹ�ó������еĿ��п��ջ
	//���㵱ǰջ��
	int top;	//ջ��ָ��
	if (superblock->s_free_BLOCK_NUM == 0) 
	{	//ʣ����п���Ϊ0
		printf("û�п��п���Է���\n");
		return -1;	//û�пɷ���Ŀ��п飬����-1
	}
	else 
	{	//����ʣ���
		top = (superblock->s_free_BLOCK_NUM - 1) % superblock->s_blocks_per_group;
	}
	//��ջ��ȡ��
	//�������ջ�ף�����ǰ��ŵ�ַ���أ���Ϊջ�׿�ţ�����ջ��ָ����¿��п��ջ����ԭ����ջ
	int retAddr;

	if (top == 0) 
	{
		retAddr = superblock->s_free_addr;
		superblock->s_free_addr = superblock->s_free[0];	//ȡ����һ�����п��п��ջ�Ŀ��п��λ�ã����¿��п��ջָ��

		//ȡ����Ӧ���п����ݣ�����ԭ���Ŀ��п��ջ

		//ȡ����һ�����п��ջ������ԭ����
		fseek(fr, superblock->s_free_addr, SEEK_SET);
		fread(superblock->s_free, sizeof(superblock->s_free), 1, fr);
		fflush(fr);
		superblock->s_free_BLOCK_NUM--;
	}
	else
	{	//�����Ϊջ�ף���ջ��ָ��ĵ�ַ���أ�ջ��ָ��-1.
		retAddr = superblock->s_free[top];	//���淵�ص�ַ
		superblock->s_free[top] = -1;	//��ջ��
		top--;		//ջ��ָ��-1
		superblock->s_free_BLOCK_NUM--;	//���п���-1

	}

	//���³�����
	fseek(fw, Superblock_StartAddr, SEEK_SET);
	fwrite(superblock, sizeof(SuperBlock), 1, fw);
	fflush(fw);

	//����blockλͼ
	block_bitmap[(retAddr - Block_StartAddr) / BLOCK_SIZE] = 1;
	fseek(fw, (retAddr - Block_StartAddr) / BLOCK_SIZE + BlockBitmap_StartAddr, SEEK_SET);	//(retAddr-Block_StartAddr)/BLOCK_SIZEΪ�ڼ������п�
	fwrite(&block_bitmap[(retAddr - Block_StartAddr) / BLOCK_SIZE], sizeof(bool), 1, fw);
	fflush(fw);

	return retAddr;

}

bool mkdir(int parinoAddr, const char name[])	//Ŀ¼������������������һ��Ŀ¼�ļ�inode��ַ ,Ҫ������Ŀ¼��
{
	if (strlen(name) >= MAX_NAME_SIZE) 
	{
		printf("�������Ŀ¼������\n");
		return false;
	}

	DirItem dirlist[32];	//��ʱĿ¼�嵥

	//�������ַȡ��inode
	Inode cur;
	fseek(fr, parinoAddr, SEEK_SET);
	fread(&cur, sizeof(Inode), 1, fr);

	int i = 0;
	int cnt = cur.i_cnt + 1;	//Ŀ¼����
	int posi = -1, posj = -1;
	while (i < 320) {
		//320��Ŀ¼��֮�ڣ�����ֱ����ֱ�ӿ�����
		int dno = i / 32;	//�ڵڼ���ֱ�ӿ���,һ��ʮ��ֱ�ӿ�

		if (cur.i_dirBlock[dno] == -1) {//û��ʹ��ֱ�ӿ�
			i += 32;
			continue;
		}
		//ȡ�����ֱ�ӿ飬Ҫ�����Ŀ¼��Ŀ��λ��
		fseek(fr, cur.i_dirBlock[dno], SEEK_SET);
		fread(dirlist, sizeof(dirlist), 1, fr);
		fflush(fr);

		//����ô��̿��е�����Ŀ¼��
		int j;
		for (j = 0; j < 16; j++) {

			if (strcmp(dirlist[j].itemName, name) == 0) {
				Inode tmp;
				fseek(fr, dirlist[j].inodeAddr, SEEK_SET);
				fread(&tmp, sizeof(Inode), 1, fr);
				//if (((tmp.i_mode >> 9) & 1) == 1) {	//����Ŀ¼
				//	printf("Ŀ¼�Ѵ���\n");
				//	return false;
				//}
			}
			else if (strcmp(dirlist[j].itemName, "") == 0) {
				//�ҵ�һ�����м�¼������Ŀ¼���������λ�� 
				//��¼���λ��
				if (posi == -1) {
					posi = dno;
					posj = j;
				}

			}
			i++;
		}

	}

	if (posi != -1) 
	{	//�ҵ��������λ��

		//ȡ�����ֱ�ӿ飬Ҫ�����Ŀ¼��Ŀ��λ��
		fseek(fr, cur.i_dirBlock[posi], SEEK_SET);
		fread(dirlist, sizeof(dirlist), 1, fr);
		fflush(fr);

		//�������Ŀ¼��
		strcpy(dirlist[posj].itemName, name);	//Ŀ¼��
		//д��������¼ "." ".."���ֱ�ָ��ǰinode�ڵ��ַ���͸�inode�ڵ�
		int chiinoAddr = ialloc();	//���䵱ǰ�ڵ��ַ 
		if (chiinoAddr == -1) 
		{
			printf("inode����ʧ��\n");
			return false;
		}
		dirlist[posj].inodeAddr = chiinoAddr; //������µ�Ŀ¼�����inode��ַ

			//��������Ŀ��inode
		Inode p;
		p.i_ino = (chiinoAddr - Inode_StartAddr) / superblock->s_INODE_SIZE;
		p.i_atime = time(NULL);
		p.i_ctime = time(NULL);
		p.i_mtime = time(NULL);
		p.i_cnt = 2;	//�������ǰĿ¼,"."��".."

			//�������inode�Ĵ��̿飬�ڴ��̺���д��������¼ . �� ..
		int curblockAddr = balloc();
		if (curblockAddr == -1) 
		{
			printf("block����ʧ��\n");
			return false;
		}
		DirItem dirlist2[32] = { 0 };	//��ʱĿ¼���б� - 2
		strcpy(dirlist2[0].itemName, ".");
		strcpy(dirlist2[1].itemName, "..");
		dirlist2[0].inodeAddr = chiinoAddr;	//��ǰĿ¼inode��ַ
		dirlist2[1].inodeAddr = parinoAddr;	//��Ŀ¼inode��ַ

		//д�뵽��ǰĿ¼�Ĵ��̿�
		fseek(fw, curblockAddr, SEEK_SET);
		fwrite(dirlist2, sizeof(dirlist2), 1, fw);

		p.i_dirBlock[0] = curblockAddr;
		int k;
		for (k = 1; k < 10; k++) {
			p.i_dirBlock[k] = -1;
		}
		p.i_size = superblock->s_BLOCK_SIZE;
		p.i_indirBlock_1 = -1;	//ûʹ��һ����ӿ�
		//p.i_mode = MODE_DIR | DIR_DEF_PERMISSION;

		//��inodeд�뵽�����inode��ַ
		fseek(fw, chiinoAddr, SEEK_SET);
		fwrite(&p, sizeof(Inode), 1, fw);

		//����ǰĿ¼�Ĵ��̿�д��
		fseek(fw, cur.i_dirBlock[posi], SEEK_SET);
		fwrite(dirlist, sizeof(dirlist), 1, fw);

		//д��inode
		cur.i_cnt++;
		fseek(fw, parinoAddr, SEEK_SET);
		fwrite(&cur, sizeof(Inode), 1, fw);
		fflush(fw);

		return true;
	}
	else 
	{
		printf("û�ҵ�����Ŀ¼��,Ŀ¼����ʧ��");
		return false;
	}
}

bool Install()	//��װ�ļ�ϵͳ������������ļ��еĹؼ���Ϣ�糬������뵽�ڴ�
{
	//��д��������ļ�����ȡ�����飬��ȡinodeλͼ��blockλͼ����ȡ��Ŀ¼����ȡetcĿ¼����ȡ����ԱadminĿ¼����ȡ�û�xiaoĿ¼����ȡ�û�passwd�ļ���

	//��ȡ������
	fseek(fr, Superblock_StartAddr, SEEK_SET);//ת��ָ��
	fread(superblock, sizeof(SuperBlock), 1, fr);//��ȡָ������

	//��ȡinodeλͼ
	fseek(fr, InodeBitmap_StartAddr, SEEK_SET);
	fread(inode_bitmap, sizeof(inode_bitmap), 1, fr);

	//��ȡblockλͼ
	fseek(fr, BlockBitmap_StartAddr, SEEK_SET);
	fread(block_bitmap, sizeof(block_bitmap), 1, fr);

	return true;

}

void cd(int parinoAddr, char name[])	//���뵱ǰĿ¼�µ�nameĿ¼
{
	//ȡ����ǰĿ¼��inode
	Inode cur;
	fseek(fr, parinoAddr, SEEK_SET);
	fread(&cur, sizeof(Inode), 1, fr);

	//����ȡ��inode��Ӧ�Ĵ��̿飬������û������Ϊname��Ŀ¼��
	int i = 0;

	//ȡ��Ŀ¼����
	int cnt = cur.i_cnt;

	while (i < 320) {
		DirItem dirlist[32] = { 0 };
		if (cur.i_dirBlock[i / 32] == -1) {
			i += 32;
			continue;
		}
		//ȡ�����̿�
		int parblockAddr = cur.i_dirBlock[i / 32];
		fseek(fr, parblockAddr, SEEK_SET);
		fread(&dirlist, sizeof(dirlist), 1, fr);

		//����ô��̿��е�����Ŀ¼��
		int j;
		for (j = 0; j < 32; j++) {
			if (strcmp(dirlist[j].itemName, name) == 0) {
				Inode tmp;
				//ȡ����Ŀ¼���inode���жϸ�Ŀ¼����Ŀ¼�����ļ�
				fseek(fr, dirlist[j].inodeAddr, SEEK_SET);
				fread(&tmp, sizeof(Inode), 1, fr);

				if (((tmp.i_mode >> 9) & 1) == 1) {
					//�ҵ���Ŀ¼���ж��Ƿ���н���Ȩ��
					//�ҵ���Ŀ¼������Ŀ¼��������ǰĿ¼

					Cur_Dir_Addr = dirlist[j].inodeAddr;
					if (strcmp(dirlist[j].itemName, ".") == 0) {
						//��Ŀ¼������
					}
					else if (strcmp(dirlist[j].itemName, "..") == 0) {
						//��һ��Ŀ¼
						int k;
						for (k = strlen(Cur_Dir_Name); k >= 0; k--)
							if (Cur_Dir_Name[k] == '/')
								break;
						Cur_Dir_Name[k] = '\0';
						if (strlen(Cur_Dir_Name) == 0)
							Cur_Dir_Name[0] = '/', Cur_Dir_Name[1] = '\0';
					}
					else {
						if (Cur_Dir_Name[strlen(Cur_Dir_Name) - 1] != '/')
							strcat(Cur_Dir_Name, "/");
						strcat(Cur_Dir_Name, dirlist[j].itemName);
					}

					return;
				}
				else {
					//�ҵ���Ŀ¼��������Ŀ¼��������
				}

			}

			i++;
		}

	}

	//û�ҵ�
	printf("û�и�Ŀ¼\n");
	return;

}
//��
void info() //��ʾ�ļ�����
{
}
bool quit()//�˳�ϵͳ
{
	return 0;
}
void help()//��ʾ������Ϣ
{
}
//��
bool win_cp_minfs(char *a,char *b) //win��minifs��������
{
	return 0;
}
bool minifs_cp_win(char *a,char *b)//minifs��win����
{
	return 0;
}
bool minifs_cp_minifs(char *a,char *b)//minifs��minifs����
{
	return 0;
}
//��
bool del_file(int parinoAddr, char *a)//ɾ���ļ�������
{
	return 0;
}
bool del_folder(int parinoAddr, char *a)//ɾ���ļ���
{
	return 0;
}
//��
void type_txt(int parinoAddr)//��ʾ��ǰ�ļ����µ�txt
{
}
void more(int parinoAddr)//��ҳ��ʾ
{}
void att(char *b)//��ʾ�ռ��ļ�����
{
}
//��
void find(char *a)//Ѱ���ļ�
{
}
int find_n(int parinoAddr,char *a,int mode/*0Ϊ�ļ���1ΪĿ¼*/)//�ڲ�Ѱ���ļ�������inode��ַ
{
	//�������ַȡ��inode
	Inode cur;
	fseek(fr, parinoAddr, SEEK_SET);
	fread(&cur, sizeof(Inode), 1, fr);

	//ȡ��Ŀ¼����
	int cnt = cur.i_cnt;

	//����ȡ�����̿�
	int i = 0;
	while (i < 320) 
	{	//С��320
		DirItem dirlist[32] = { 0 };

		if (cur.i_dirBlock[i / 16] == -1) {
			i += 16;
			continue;
		}
		//ȡ�����̿�
		int parblockAddr = cur.i_dirBlock[i / 32];
		fseek(fr, parblockAddr, SEEK_SET);
		fread(&dirlist, sizeof(dirlist), 1, fr);

		//�ҵ�Ҫɾ����Ŀ¼
		int j;
		for (j = 0; j < 16; j++) {
			Inode tmp;
			//ȡ����Ŀ¼���inode���жϸ�Ŀ¼����Ŀ¼�����ļ�
			fseek(fr, dirlist[j].inodeAddr, SEEK_SET);
			fread(&tmp, sizeof(Inode), 1, fr);

			if (strcmp(dirlist[j].itemName, a) == 0) {
				if (((tmp.i_mode >> 9) & 1) == mode) {	//�ҵ�Ŀ¼
					//��Ŀ¼
					return dirlist[j].inodeAddr;
				}
				else {
					//����Ŀ¼������
				}
			}
			i++;
		}

	}
	return -1;
}