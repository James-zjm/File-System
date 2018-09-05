#include"pch.h"
#include"try.h"

bool Format()	//格式化一个虚拟磁盘文件
{
	int i, j;

	//初始化超级块
	superblock->s_INODE_NUM = INODE_NUM;
	superblock->s_BLOCK_NUM = BLOCK_NUM;
	superblock->s_SUPERBLOCK_SIZE = sizeof(SuperBlock);//560
	superblock->s_INODE_SIZE = INODE_SIZE;
	superblock->s_BLOCK_SIZE = BLOCK_SIZE;
	superblock->s_free_INODE_NUM = INODE_NUM;
	superblock->s_free_BLOCK_NUM = BLOCK_NUM;
	superblock->s_blocks_per_group = BLOCKS_PER_GROUP;
	superblock->s_free_addr = Block_StartAddr;	//空闲块堆栈指针为第一块block
	superblock->s_Superblock_StartAddr = Superblock_StartAddr;
	superblock->s_BlockBitmap_StartAddr = BlockBitmap_StartAddr;
	superblock->s_InodeBitmap_StartAddr = InodeBitmap_StartAddr;
	superblock->s_Block_StartAddr = Block_StartAddr;
	superblock->s_Inode_StartAddr = Inode_StartAddr;
	//空闲块堆栈在后面赋值

	//初始化inode位图
	memset(inode_bitmap, 0, sizeof(inode_bitmap));
	fseek(fw, InodeBitmap_StartAddr, SEEK_SET);
	fwrite(inode_bitmap, sizeof(inode_bitmap), 1, fw);

	//初始化block位图
	memset(block_bitmap, 0, sizeof(block_bitmap));
	fseek(fw, BlockBitmap_StartAddr, SEEK_SET);
	fwrite(block_bitmap, sizeof(block_bitmap), 1, fw);

	//初始化磁盘块区，根据成组链接法组织	
	for (i = BLOCK_NUM / BLOCKS_PER_GROUP - 1; i >= 0; i--) 
	{	//一共INODE_NUM/BLOCKS_PER_GROUP 8192 组，一组FREESTACKNUM（128）个磁盘块 ，第一个磁盘块作为索引
		if (i == BLOCK_NUM / BLOCKS_PER_GROUP - 1)
			superblock->s_free[0] = -1;	//没有下一个空闲块了
		else
			superblock->s_free[0] = Block_StartAddr + (i + 1)*BLOCKS_PER_GROUP*BLOCK_SIZE;	//指向下一个空闲块
		for (j = 1; j < BLOCKS_PER_GROUP; j++) 
		{
			superblock->s_free[j] = Block_StartAddr + (i*BLOCKS_PER_GROUP + j)*BLOCK_SIZE;
		}
		fseek(fw, Block_StartAddr + i * BLOCKS_PER_GROUP*BLOCK_SIZE, SEEK_SET);
		fwrite(superblock->s_free, sizeof(superblock->s_free), 1, fw);	//填满这个磁盘块，512字节
	}
	//超级块写入到虚拟磁盘文件
	fseek(fw, Superblock_StartAddr, SEEK_SET);
	fwrite(superblock, sizeof(SuperBlock), 1, fw);

	fflush(fw);//强迫将缓冲区内的数据写回参数stream 指定的文件中

	//读取inode位图
	fseek(fr, InodeBitmap_StartAddr, SEEK_SET);
	fread(inode_bitmap, sizeof(inode_bitmap), 1, fr);

	//读取block位图
	fseek(fr, BlockBitmap_StartAddr, SEEK_SET);
	fread(block_bitmap, sizeof(block_bitmap), 1, fr);

	fflush(fr);

	//创建根目录 "/"
	Inode cur;

	//申请inode
	int inoAddr = ialloc();

	//给这个inode申请磁盘块
	int blockAddr = balloc();

	//在这个磁盘块里加入一个条目 "."
	DirItem dirlist[32] = { 0 };
	strcpy(dirlist[0].itemName, ".");
	dirlist[0].inodeAddr = inoAddr;

	//写回磁盘块
	fseek(fw, blockAddr, SEEK_SET);
	fwrite(dirlist, sizeof(dirlist), 1, fw);

	//给inode赋值
	cur.i_ino = 0;
	cur.i_atime = time(NULL);
	cur.i_ctime = time(NULL);
	cur.i_mtime = time(NULL);
	cur.i_cnt = 1;	//一个项，当前目录,"."根目录
	cur.i_dirBlock[0] = blockAddr;
	for (i = 1; i < 10; i++) {
		cur.i_dirBlock[i] = -1;
	}
	cur.i_size = superblock->s_BLOCK_SIZE;
	cur.i_indirBlock_1 = -1;	//没使用一级间接块
	cur.i_indirBlock_2 = -1;
	cur.i_indirBlock_3 = -1;
	cur.i_mode = MODE_DIR ;

	//写回inode
	fseek(fw, inoAddr, SEEK_SET);
	fwrite(&cur, sizeof(Inode), 1, fw);
	fflush(fw);

	return true;
}

int ialloc()	//分配i节点区函数，返回inode地址
{
	//在inode位图中顺序查找空闲的inode，找到则返回inode地址。函数结束。
	if (superblock->s_free_INODE_NUM == 0) 
	{
		printf("没有空闲inode可以分配\n");
		return -1;
	}
	else 
	{

		//顺序查找空闲的inode
		int i;
		for (i = 0; i < superblock->s_INODE_NUM; i++) 
		{
			if (inode_bitmap[i] == 0)	//找到空闲inode
				break;
		}


		//更新超级块
		superblock->s_free_INODE_NUM--;	//空闲inode数-1
		fseek(fw, Superblock_StartAddr, SEEK_SET);
		fwrite(superblock, sizeof(SuperBlock), 1, fw);

		//更新inode位图
		inode_bitmap[i] = 1;
		fseek(fw, InodeBitmap_StartAddr + i, SEEK_SET);
		fwrite(&inode_bitmap[i], sizeof(bool), 1, fw);
		fflush(fw);

		return Inode_StartAddr + i * superblock->s_INODE_SIZE;
	}
}

int balloc()	//磁盘块分配函数
{
	//使用超级块中的空闲块堆栈
	//计算当前栈顶
	int top;	//栈顶指针
	if (superblock->s_free_BLOCK_NUM == 0) 
	{	//剩余空闲块数为0
		printf("没有空闲块可以分配\n");
		return -1;	//没有可分配的空闲块，返回-1
	}
	else 
	{	//还有剩余块
		top = (superblock->s_free_BLOCK_NUM - 1) % superblock->s_blocks_per_group;
	}
	//将栈顶取出
	//如果已是栈底，将当前块号地址返回，即为栈底块号，并将栈底指向的新空闲块堆栈覆盖原来的栈
	int retAddr;

	if (top == 0) 
	{
		retAddr = superblock->s_free_addr;
		superblock->s_free_addr = superblock->s_free[0];	//取出下一个存有空闲块堆栈的空闲块的位置，更新空闲块堆栈指针

		//取出对应空闲块内容，覆盖原来的空闲块堆栈

		//取出下一个空闲块堆栈，覆盖原来的
		fseek(fr, superblock->s_free_addr, SEEK_SET);
		fread(superblock->s_free, sizeof(superblock->s_free), 1, fr);
		fflush(fr);
		superblock->s_free_BLOCK_NUM--;
	}
	else
	{	//如果不为栈底，则将栈顶指向的地址返回，栈顶指针-1.
		retAddr = superblock->s_free[top];	//保存返回地址
		superblock->s_free[top] = -1;	//清栈顶
		top--;		//栈顶指针-1
		superblock->s_free_BLOCK_NUM--;	//空闲块数-1

	}

	//更新超级块
	fseek(fw, Superblock_StartAddr, SEEK_SET);
	fwrite(superblock, sizeof(SuperBlock), 1, fw);
	fflush(fw);

	//更新block位图
	block_bitmap[(retAddr - Block_StartAddr) / BLOCK_SIZE] = 1;
	fseek(fw, (retAddr - Block_StartAddr) / BLOCK_SIZE + BlockBitmap_StartAddr, SEEK_SET);	//(retAddr-Block_StartAddr)/BLOCK_SIZE为第几个空闲块
	fwrite(&block_bitmap[(retAddr - Block_StartAddr) / BLOCK_SIZE], sizeof(bool), 1, fw);
	fflush(fw);

	return retAddr;

}

bool mkdir(int parinoAddr, const char name[])	//目录创建函数。参数：上一层目录文件inode地址 ,要创建的目录名
{
	if (strlen(name) >= MAX_NAME_SIZE) 
	{
		printf("超过最大目录名长度\n");
		return false;
	}

	DirItem dirlist[32];	//临时目录清单

	//从这个地址取出inode
	Inode cur;
	fseek(fr, parinoAddr, SEEK_SET);
	fread(&cur, sizeof(Inode), 1, fr);

	int i = 0;
	int cnt = cur.i_cnt + 1;	//目录项数
	int posi = -1, posj = -1;
	while (i < 320) {
		//320个目录项之内，可以直接在直接块里找
		int dno = i / 32;	//在第几个直接块里,一共十个直接块

		if (cur.i_dirBlock[dno] == -1) {//没有使用直接块
			i += 32;
			continue;
		}
		//取出这个直接块，要加入的目录条目的位置
		fseek(fr, cur.i_dirBlock[dno], SEEK_SET);
		fread(dirlist, sizeof(dirlist), 1, fr);
		fflush(fr);

		//输出该磁盘块中的所有目录项
		int j;
		for (j = 0; j < 16; j++) {

			if (strcmp(dirlist[j].itemName, name) == 0) {
				Inode tmp;
				fseek(fr, dirlist[j].inodeAddr, SEEK_SET);
				fread(&tmp, sizeof(Inode), 1, fr);
				//if (((tmp.i_mode >> 9) & 1) == 1) {	//不是目录
				//	printf("目录已存在\n");
				//	return false;
				//}
			}
			else if (strcmp(dirlist[j].itemName, "") == 0) {
				//找到一个空闲记录，将新目录创建到这个位置 
				//记录这个位置
				if (posi == -1) {
					posi = dno;
					posj = j;
				}

			}
			i++;
		}

	}

	if (posi != -1) 
	{	//找到这个空闲位置

		//取出这个直接块，要加入的目录条目的位置
		fseek(fr, cur.i_dirBlock[posi], SEEK_SET);
		fread(dirlist, sizeof(dirlist), 1, fr);
		fflush(fr);

		//创建这个目录项
		strcpy(dirlist[posj].itemName, name);	//目录名
		//写入两条记录 "." ".."，分别指向当前inode节点地址，和父inode节点
		int chiinoAddr = ialloc();	//分配当前节点地址 
		if (chiinoAddr == -1) 
		{
			printf("inode分配失败\n");
			return false;
		}
		dirlist[posj].inodeAddr = chiinoAddr; //给这个新的目录分配的inode地址

			//设置新条目的inode
		Inode p;
		p.i_ino = (chiinoAddr - Inode_StartAddr) / superblock->s_INODE_SIZE;
		p.i_atime = time(NULL);
		p.i_ctime = time(NULL);
		p.i_mtime = time(NULL);
		p.i_cnt = 2;	//两个项，当前目录,"."和".."

			//分配这个inode的磁盘块，在磁盘号中写入两条记录 . 和 ..
		int curblockAddr = balloc();
		if (curblockAddr == -1) 
		{
			printf("block分配失败\n");
			return false;
		}
		DirItem dirlist2[32] = { 0 };	//临时目录项列表 - 2
		strcpy(dirlist2[0].itemName, ".");
		strcpy(dirlist2[1].itemName, "..");
		dirlist2[0].inodeAddr = chiinoAddr;	//当前目录inode地址
		dirlist2[1].inodeAddr = parinoAddr;	//父目录inode地址

		//写入到当前目录的磁盘块
		fseek(fw, curblockAddr, SEEK_SET);
		fwrite(dirlist2, sizeof(dirlist2), 1, fw);

		p.i_dirBlock[0] = curblockAddr;
		int k;
		for (k = 1; k < 10; k++) {
			p.i_dirBlock[k] = -1;
		}
		p.i_size = superblock->s_BLOCK_SIZE;
		p.i_indirBlock_1 = -1;	//没使用一级间接块
		//p.i_mode = MODE_DIR | DIR_DEF_PERMISSION;

		//将inode写入到申请的inode地址
		fseek(fw, chiinoAddr, SEEK_SET);
		fwrite(&p, sizeof(Inode), 1, fw);

		//将当前目录的磁盘块写回
		fseek(fw, cur.i_dirBlock[posi], SEEK_SET);
		fwrite(dirlist, sizeof(dirlist), 1, fw);

		//写回inode
		cur.i_cnt++;
		fseek(fw, parinoAddr, SEEK_SET);
		fwrite(&cur, sizeof(Inode), 1, fw);
		fflush(fw);

		return true;
	}
	else 
	{
		printf("没找到空闲目录项,目录创建失败");
		return false;
	}
}

bool Install()	//安装文件系统，将虚拟磁盘文件中的关键信息如超级块读入到内存
{
	//读写虚拟磁盘文件，读取超级块，读取inode位图，block位图，读取主目录，读取etc目录，读取管理员admin目录，读取用户xiao目录，读取用户passwd文件。

	//读取超级块
	fseek(fr, Superblock_StartAddr, SEEK_SET);//转移指针
	fread(superblock, sizeof(SuperBlock), 1, fr);//读取指定内容

	//读取inode位图
	fseek(fr, InodeBitmap_StartAddr, SEEK_SET);
	fread(inode_bitmap, sizeof(inode_bitmap), 1, fr);

	//读取block位图
	fseek(fr, BlockBitmap_StartAddr, SEEK_SET);
	fread(block_bitmap, sizeof(block_bitmap), 1, fr);

	return true;

}

void cd(int parinoAddr, char name[])	//进入当前目录下的name目录
{
	//取出当前目录的inode
	Inode cur;
	fseek(fr, parinoAddr, SEEK_SET);
	fread(&cur, sizeof(Inode), 1, fr);

	//依次取出inode对应的磁盘块，查找有没有名字为name的目录项
	int i = 0;

	//取出目录项数
	int cnt = cur.i_cnt;

	while (i < 320) {
		DirItem dirlist[32] = { 0 };
		if (cur.i_dirBlock[i / 32] == -1) {
			i += 32;
			continue;
		}
		//取出磁盘块
		int parblockAddr = cur.i_dirBlock[i / 32];
		fseek(fr, parblockAddr, SEEK_SET);
		fread(&dirlist, sizeof(dirlist), 1, fr);

		//输出该磁盘块中的所有目录项
		int j;
		for (j = 0; j < 32; j++) {
			if (strcmp(dirlist[j].itemName, name) == 0) {
				Inode tmp;
				//取出该目录项的inode，判断该目录项是目录还是文件
				fseek(fr, dirlist[j].inodeAddr, SEEK_SET);
				fread(&tmp, sizeof(Inode), 1, fr);

				if (((tmp.i_mode >> 9) & 1) == 1) {
					//找到该目录，判断是否具有进入权限
					//找到该目录项，如果是目录，更换当前目录

					Cur_Dir_Addr = dirlist[j].inodeAddr;
					if (strcmp(dirlist[j].itemName, ".") == 0) {
						//本目录，不动
					}
					else if (strcmp(dirlist[j].itemName, "..") == 0) {
						//上一次目录
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
					//找到该目录项，如果不是目录，继续找
				}

			}

			i++;
		}

	}

	//没找到
	printf("没有该目录\n");
	return;

}
//李
void info() //显示文件属性
{
}
bool quit()//退出系统
{
	return 0;
}
void help()//显示帮助信息
{
}
//朗
bool win_cp_minfs(char *a,char *b) //win向minifs拷贝命令
{
	return 0;
}
bool minifs_cp_win(char *a,char *b)//minifs向win拷贝
{
	return 0;
}
bool minifs_cp_minifs(char *a,char *b)//minifs向minifs拷贝
{
	return 0;
}
//张
bool del_file(int parinoAddr, char *a)//删除文件，名字
{
	return 0;
}
bool del_folder(int parinoAddr, char *a)//删除文件夹
{
	return 0;
}
//何
void type_txt(int parinoAddr)//显示当前文件夹下的txt
{
}
void more(int parinoAddr)//分页显示
{}
void att(char *b)//显示空间文件属性
{
}
//王
void find(char *a)//寻找文件
{
}
int find_n(int parinoAddr,char *a,int mode/*0为文件，1为目录*/)//内部寻找文件，返回inode地址
{
	//从这个地址取出inode
	Inode cur;
	fseek(fr, parinoAddr, SEEK_SET);
	fread(&cur, sizeof(Inode), 1, fr);

	//取出目录项数
	int cnt = cur.i_cnt;

	//依次取出磁盘块
	int i = 0;
	while (i < 320) 
	{	//小于320
		DirItem dirlist[32] = { 0 };

		if (cur.i_dirBlock[i / 16] == -1) {
			i += 16;
			continue;
		}
		//取出磁盘块
		int parblockAddr = cur.i_dirBlock[i / 32];
		fseek(fr, parblockAddr, SEEK_SET);
		fread(&dirlist, sizeof(dirlist), 1, fr);

		//找到要删除的目录
		int j;
		for (j = 0; j < 16; j++) {
			Inode tmp;
			//取出该目录项的inode，判断该目录项是目录还是文件
			fseek(fr, dirlist[j].inodeAddr, SEEK_SET);
			fread(&tmp, sizeof(Inode), 1, fr);

			if (strcmp(dirlist[j].itemName, a) == 0) {
				if (((tmp.i_mode >> 9) & 1) == mode) {	//找到目录
					//是目录
					return dirlist[j].inodeAddr;
				}
				else {
					//不是目录，不管
				}
			}
			i++;
		}

	}
	return -1;
}