#include "head.h"
#include <cstdio>

char	choice;
vector<string> vc_of_str;
string	s1, s2;
int		cur_loc; // 当前目录
char	tmp[2 * BLKSIZE]; // 缓冲区
User	user; // 当前的用户
char	bitmap[BLKNUM];	// 位图数组
Inode	inode_array[INODENUM]; // i节点数组
File_table file_array[FILENUM]; // 打开文件表数组
char	image_name[10] = "image.dat"; // 文件系统名称
FILE*	fp; // 文件指针

// 支持命令 "help", "cd", "dir", "mkdir", "touch", "open","read", "write", "close", "delete", "logout", "clear", "format","quit","rd"
const string Commands[] = { "help", "cd", "ls", "mkdir", "touch", "open","cat", "vi", "close", "rm", "su", "clear", "format","exit","rmdir","df" };

// bin/xx 给出进入bin即可
int readby() {	//根据当前目录和第二个参数确定转过去的目录
	int result_cur = 0; string s = s2;
	if (s.find('/') != -1) {
		s = s.substr(0, s.find_last_of('/') + 1);
	}
	else {
		s = "";
	}
	int tmp_cur = cur_loc;
	vector<string>v;
	while (s.find('/') != -1) {
		v.push_back(s.substr(0, s.find_first_of('/')));
		s = s.substr(s.find_first_of('/') + 1);
	}
	if (v.size() == 0) {
		return cur_loc;
	}if (v[0].length() == 0) {
		tmp_cur = 0;
	}
	else if (v[0] == "..") {
		if (inode_array[cur_loc].inum > 0) {
			tmp_cur = inode_array[cur_loc].iparent;
		}
	}
	else {
		int i;
		for (i = 0; i < INODENUM; i++) {
			if ((inode_array[i].inum > 0) &&
				(inode_array[i].iparent == cur_loc) &&
				(inode_array[i].type == 'd') &&
				inode_array[i].file_name == v[0]) {
				break;
			}
		}
		if (i == INODENUM) {
			return -1;
		}
		else {
			tmp_cur = i;
		}
	}
	int i;
	for (unsigned int count = 1; count < v.size(); count++) {
		for (i = 0; i < INODENUM; i++) {
			if ((inode_array[i].inum > 0) &&//是否为空
				(inode_array[i].iparent == tmp_cur) &&
				(inode_array[i].type == 'd') &&
				inode_array[i].file_name == v[count]) {
				break;
			}
		}
		if (i == INODENUM) {
			return -1;
		}
		else {
			tmp_cur = i;
		}
	}
	result_cur = tmp_cur;
	return result_cur;
}

//创建磁盘映像，并将所有用户和文件清除
void format(void)
{
	int   i;
	printf("Formating filesystem...\n");
	printf("All the old data will lost!!!\n");
	printf("Are you sure format the fileSystem?(Y/N)?");
	scanf("%c", &choice);
	gets(tmp);   //更改
	if ((choice == 'y') || (choice == 'Y'))
	{
		// 打开映像文件
		fp = fopen(image_name, "w+b");
		if (fp == NULL)
		{
			printf("Can't open  file %s\n", image_name);
			getchar();
			exit(-1);
		}
		//成功打开后，位图数组置0
		for (i = 0; i < BLKSIZE; i++)
			fputc('0', fp);
		Inode inode;
		inode.inum = 0;
		inode.iparent = 0;
		inode.length = 0;
		inode.address[0] = -1;
		inode.address[1] = -1;
		inode.type = 'd';
		strcpy(inode.file_name, "/");
		strcpy(inode.user_name, "all");
		fwrite(&inode, sizeof(Inode), 1, fp);
		inode.inum = -1;
		//对32个节点进行格式化
		for (i = 0; i < 31; i++)
			fwrite(&inode, sizeof(Inode), 1, fp);
		//数据块进行格式化
		for (i = 0; i < BLKNUM * BLKSIZE; i++)
			fputc('\0', fp);
		fclose(fp);
		//初始化用户文件
		fp = fopen("user.txt", "w+");
		if (fp == NULL)
		{
			printf("Can't create file %s\n", "user.txt");
			exit(-1);
		}
		fclose(fp);
		printf("Filesystem created successful.Please login!\n");
	} else
		quit();
	return;
}

/*功能: 用户登陆，如果是新用户则创建用户*/
void login() {
	// 先检查文件系统是否存在
	if ((fp = fopen("user.txt", "r+")) == NULL) {
		printf("Unable to load filesystem, trying to repair now...\n");
		format(); 
		login();
	}
	int  flag = 0;
	char user_name[10];
	char password[10];
	char choice;    //选择是否（y/n）
	do {
		// 输入用户名、密码
		printf("username:");
		gets(user_name);
		printf("password:");
		char* p = password;
		while (*p = getch()) {
			if (*p == 0x0d) //当输入回车时
			{
				*p = '\0'; //将输入的回车符转换成字符串结束符
				break;
			}
			printf("*");   //将输入的密码以"*"号显示
			p++;
		}
		// 校验用户名、密码
		while (!feof(fp)) {
			fread(&user, sizeof(User), 1, fp);
			// 已经存在的用户, 且密码正确
			if (!strcmp(user.user_name, user_name) &&
				!strcmp(user.password, password)) {
				fclose(fp);
				printf("\n");
				return;//校验成功，直接跳出登陆函数 
			}
			// 已经存在的用户, 但密码错误
			else if (!strcmp(user.user_name, user_name)) {
				printf("\nPassword incorrect!\n");
				flag = 1;    //密码错误，重新登陆 
				fclose(fp);
				break;
			}
			// 不存在的用户名
			else {
				printf("\nThis user is not exist.\n");
				break;//用户不存在，先跳出循环创建新用户 
			}
		}
	} while (flag);
	// 创建新用户
	if (flag == 0) {
		printf("\nDo you want to creat a new user?(y/n):");
		scanf("%c", &choice);
		if ((choice == 'y') || (choice == 'Y')) {
			strcpy(user.user_name, user_name);
			strcpy(user.password, password);
			fwrite(&user, sizeof(User), 1, fp);
			fclose(fp);
			return;
		}
		else
			quit();
	}
}

// 功能: 将所有i节点读入内存
void init(void)
{
	int   i;
	if ((fp = fopen(image_name, "r+b")) == NULL)
	{
		printf("Can't open file %s.\n", image_name);
		exit(-1);
	}
	// 读入位图
	for (i = 0; i < BLKNUM; i++)
		bitmap[i] = fgetc(fp);
	// 显示位图
	// 读入i节点信息
	for (i = 0; i < INODENUM; i++)
		fread(&inode_array[i], sizeof(Inode), 1, fp);
	// 显示i节点
	// 当前目录为根目录
	cur_loc = 0;
	// 初始化打开文件表
	for (i = 0; i < FILENUM; i++)
		file_array[i].inum = -1;
}

void df() {
	int count_inode = 0, count_mem = 0;
	for (int i = 0; i < INODENUM; i++) {
		if (inode_array[i].inum > 0)count_inode++;
	}
	for (int i = 0; i < BLKNUM; i++) {
		if (bitmap[i] == '1')count_mem++;
	}
	cout << "Inode use:" << count_inode << "/" << INODENUM << endl;
	cout << "Store use:" << count_mem << "/" << BLKNUM << endl;
}

void StrListForCom() {
	vc_of_str.clear();
	vc_of_str.push_back("cd");
	vc_of_str.push_back("mkdir");
	vc_of_str.push_back("ls");
	vc_of_str.push_back("vi");
	vc_of_str.push_back("sudo");
	vc_of_str.push_back("rm");
	vc_of_str.push_back("touch");
	vc_of_str.push_back("help");
	vc_of_str.push_back("cat");
	vc_of_str.push_back("clear");
	vc_of_str.push_back("rmdir");
	vc_of_str.push_back("df");
}

void StrListForAdd() {
	vc_of_str.clear();
	int tmp_cur;
	tmp_cur = readby();
	for (int i = 0; i < INODENUM; i++) {
		if ((inode_array[i].inum > 0) &&
			(inode_array[i].iparent == tmp_cur)) {
			if (inode_array[i].type == 'd' && !strcmp(inode_array[i].user_name, user.user_name))
			{
				string tmp = inode_array[i].file_name;
				tmp += '/';
				vc_of_str.push_back(tmp);
			}
			if (inode_array[i].type == '-' && !strcmp(inode_array[i].user_name, user.user_name))
			{
				vc_of_str.push_back(inode_array[i].file_name);
			}
		}

	}
}

// 结果: 0-14为系统命令, 15为命令错误
int analyse()
{
	string s = ""; s1 = ""; s2 = "";
	int tabcount = 0;
	int res = 0;
	while (1) {
		s1 = s;
		if (s.find(' ') == -1)s2 = "";
		else {
			while (s1.length() > 0 && s1[s1.length() - 1] == ' ') {
				s1 = s1.substr(0, s1.length() - 1);
			}
			while (s1.length() > 0 && s1[0] == ' ') {
				s1 = s1.substr(1);
			}
			if (s1.find(' ') == -1)s2 = "";
			else
				s2 = s1.substr(s1.find_first_of(' ') + 1);
			while (s2.length() > 0 && s2[s2.length() - 1] == ' ') {
				s2 = s2.substr(0, s2.length() - 1);
			}
			while (s2.length() > 0 && s2[0] == ' ') {
				s2 = s2.substr(1);
			}
			s1 = s1.substr(0, s1.find_first_of(' '));
		}
		int ch = _getch();
		if (ch == 8) {				//退格
			if (!s.empty()) {
				printf("%c", 8);
				printf(" ");
				printf("%c", 8);
				s = s.substr(0, s.length() - 1);
			}
		}
		else if (ch == 13) {		//回车
			for (res = 0; res < 16; res++) {
				if (s1 == Commands[res])break;
			}
			break;
		}
		else if (ch == 9) {			//tab
			int count = 0; vector<int>v;
			string tstr;
			if (s.find(' ') != -1) {
				tstr = s.substr(s.find_last_of(' ') + 1);
				if (tstr.find('/') != -1) {
					tstr = tstr.substr(tstr.find_last_of('/') + 1);
				}
				StrListForAdd();
			}
			else {
				tstr = s;
				StrListForCom();
			}
			for (unsigned int i = 0; i < vc_of_str.size(); i++) {
				if (vc_of_str[i].length() >= tstr.length() && vc_of_str[i].substr(0, tstr.length()) == tstr) {
					count++; v.push_back(i);
				}
			}
			//cout << "count:" << count<<endl;
			//cout << "tstr:" << tstr<<endl;
			if (count < 1) {
				if (s.find(' ') == -1) {
					s.push_back(' ');
					printf(" ");
				}
				tabcount = -1;
			}
			if (count == 1) {
				for (unsigned int i = tstr.length(); i < vc_of_str[v[0]].length(); i++) {
					s.push_back(vc_of_str[v[0]][i]);
					printf("%c", vc_of_str[v[0]][i]);
				}
				if (s.find(' ') == -1) {
					s.push_back(' ');
					printf(" ");
				}
				tabcount = -1;
			}
			if (count > 1 && tabcount) {
				cout << "\n";
				cout << vc_of_str[v[0]];
				for (unsigned int i = 1; i < v.size(); i++) {
					cout << "    " << vc_of_str[v[i]];
				}
				cout << endl;
				pathset();
				cout << s;
				tabcount = -1;
			}

		}
		else if (ch == ' ') {
			printf("%c", ch);
			s.push_back(ch);
		}
		else {
			printf("%c", ch);
			s.push_back(ch);
		}
		//用于处理按两次tab
		if (ch == 9) {
			tabcount++;
		}
		else {
			tabcount = 0;
		}
	}
	if (s1 == "") {
		return -1;
	}
	printf("\n");
	return res;
}

// 功能: 将num号i节点保存到hd.dat
void save_inode(int num)
{
	if ((fp = fopen(image_name, "r+b")) == NULL)
	{
		printf("Can't open file %s\n", image_name);
		exit(-1);
	}
	fseek(fp, 512 + num * sizeof(Inode), SEEK_SET);
	fwrite(&inode_array[num], sizeof(Inode), 1, fp);
	fclose(fp);
}

// 功能: 申请一个数据块
int get_blknum(void)
{
	int i;
	for (i = 0; i < BLKNUM; i++)
		if (bitmap[i] == '0') break;
	// 未找到空闲数据块
	if (i == BLKNUM)
	{
		printf("Data area is full.\n");
		exit(-1);
	}
	bitmap[i] = '1';
	if ((fp = fopen(image_name, "r+b")) == NULL)
	{
		printf("Can't open file %s\n", image_name);
		exit(-1);
	}
	fseek(fp, i, SEEK_SET);
	fputc('1', fp);
	fclose(fp);
	return i;
}

// 功能: 将i节点号为num的文件读入tmp 
void read_blk(int num)
{
	int  i, len;
	char ch;
	int  add0, add1;
	len = inode_array[num].length;
	add0 = inode_array[num].address[0];
	if (len > 512)
		add1 = inode_array[num].address[1];
	if ((fp = fopen(image_name, "r+b")) == NULL)
	{
		printf("Can't open file %s.\n", image_name);
		exit(-1);
	}
	fseek(fp, 1536 + add0 * BLKSIZE, SEEK_SET);
	ch = fgetc(fp);
	for (i = 0; (i < len) && (ch != '\0') && (i < 512); i++)
	{
		tmp[i] = ch;
		ch = fgetc(fp);
	}
	if (i >= 512)
	{
		fseek(fp, 1536 + add1 * BLKSIZE, SEEK_SET);
		ch = fgetc(fp);
		for (; (i < len) && (ch != '\0'); i++)
		{
			tmp[i] = ch;
			ch = fgetc(fp);
		}
	}
	tmp[i] = '\0';
	fclose(fp);
}

// 功能: 将tmp的内容输入hd的数据区
void write_blk(int num)
{
	int  i, len;
	int  add0, add1;
	add0 = inode_array[num].address[0];
	len = inode_array[num].length;
	if ((fp = fopen(image_name, "r+b")) == NULL)
	{
		printf("Can't open file %s.\n", image_name);
		exit(-1);
	}
	fseek(fp, 1536 + add0 * BLKSIZE, SEEK_SET);
	for (i = 0; (i < len) && (tmp[i] != '\0') && (i < 512); i++)
		fputc(tmp[i], fp);
	if (i == 512)
	{
		add1 = inode_array[num].address[1];
		fseek(fp, 1536 + add1 * BLKSIZE, SEEK_SET);
		for (; (i < len) && (tmp[i] != '\0'); i++)
			fputc(tmp[i], fp);
	}
	fputc('\0', fp);
	fclose(fp);
}

// 功能: 释放文件块号为num的文件占用的空间
void release_blk(int num)
{
	FILE* fp;
	if ((fp = fopen(image_name, "r+b")) == NULL)
	{
		printf("Can't open file %s\n", image_name);
		exit(-1);
	}
	bitmap[num] = '0';
	fseek(fp, num, SEEK_SET);
	fputc('0', fp);
	fclose(fp);
}

void help() {
	/*功能: 显示帮助命令*/
	printf("command: \n\
	help   ---  show help menu \n\
	cd     ---  change directory \n\
	clear  ---  clear the screen \n\
	ls     ---  show all the files and directories in particular directory \n\
	mkdir  ---  make a new directory   \n\
	touch  ---  create a new file \n\
	cat    ---  open and read an exist file \n\
	vi     ---  open and write something to a particular file \n\
	rm     ---  delete a exist file \n\
	su     ---  switch current user \n\
	clear  ---  clear the screen \n\
	format ---  format a exist filesystem \n\
	exit   ---  exit this system \n\
	rmdir  ---  delete a directory \n\
	df     ---  show the useage of storage \n");
}


//设置文件路径
void pathset()
{
	string s;
	if (inode_array[cur_loc].inum == 0)s = "";
	else {
		int tmp = cur_loc;
		while (inode_array[tmp].inum != 0) {
			s = inode_array[tmp].file_name + s;
			s = '/' + s;
			tmp = inode_array[tmp].iparent;
		}
	}
	cout << user.user_name << "@" << HOSTNAME << ":~" << s << "# ";
}


// 功能: 切换目录(cd .. 或者 cd dir1)
void cd()
{
	int tmp_cur;
	if (s2.length() == 0) {
		tmp_cur = 0;
	}
	else {
		if (s2[s2.length() - 1] != '/')s2 += '/';
		tmp_cur = readby();
	}
	if (tmp_cur != -1) {
		cur_loc = tmp_cur;
	}
	else {
		cout << "No Such Directory" << endl;
	}
}

// 功能: 显示当前目录下的子目录和文件(dir)
void dir(void)
{
	int tmp_cur;
	int i = 0;
	if (s2.length() == 0) {
		tmp_cur = cur_loc;
	}
	else {
		if (s2[s2.length() - 1] != '/')s2 += '/';
		tmp_cur = readby();
		if (tmp_cur == -1 || inode_array[tmp_cur].type != 'd') {
			cout << "No Such Directory" << endl;
			return;
		}
	}
	if (tmp_cur != -1 && inode_array[tmp_cur].type == 'd')
		for (i = 0; i < INODENUM; i++)
		{
			if ((inode_array[i].inum > 0) &&
				(inode_array[i].iparent == tmp_cur)
				&& !strcmp(inode_array[i].user_name, user.user_name))
			{
				if (inode_array[i].type == 'd')
				{
					printf("%-20s<DIR>\n", inode_array[i].file_name);
				}
				if (inode_array[i].type == '-')
				{
					printf("%-20s%12d bytes\n", inode_array[i].file_name, inode_array[i].length);
				}
			}
		}
}

void rm(int inode) {
	int i;
	for (i = 0; i < INODENUM; i++) {
		if (inode_array[i].iparent == inode && !strcmp(inode_array[i].user_name, user.user_name)) {
			if (inode_array[i].type == 'd')rm(i);
			else delet(i);
		}
	}
	delet(inode);
}

//rmdir dir或rmdir dir/a/b
void rmdir()
{
	if (s2.length() == 0) {
		printf("No Such Directory\n");
		return;
	}
	if (s2[s2.length() - 1] != '/')s2 += '/';
	int tmp_cur = readby();
	if (tmp_cur == -1)printf("No Such Directory\n");
	else {
		if (inode_array[tmp_cur].type != 'd') {
			printf("That's A File!\n");
		}
		else {
			int tmp = inode_array[cur_loc].iparent;
			while (tmp != 0) {
				if (tmp == tmp_cur || tmp_cur == 0) {
					printf("Can't delete your father directory\n");
					return;
				}
				tmp = inode_array[tmp].iparent;
			}
			rm(tmp_cur);
		}
	}
	return;
}

// 功能: 在当前目录下创建子目录(mkdir dir1)
void mkdir(void)
{
	int i;
	if (s2.length() == 0) {
		cout << "Please input name" << endl;
		return;
	}
	// 遍历i节点数组, 查找未用的i节点
	for (i = 0; i < INODENUM; i++) {
		if (inode_array[i].iparent == cur_loc && inode_array[i].type == 'd'
			&& inode_array[i].file_name == s2 && inode_array[i].inum > 0
			&& !strcmp(inode_array[i].user_name, user.user_name)) {
			break;
		}
	}
	if (i != INODENUM) {
		printf("There is directory having same name.\n");
		return;
	}
	for (i = 0; i < INODENUM; i++)
		if (inode_array[i].inum < 0) break;
	if (i == INODENUM)
	{
		printf("Inode is full.\n");
		exit(-1);
	}
	inode_array[i].inum = i;
	strcpy(inode_array[i].file_name, s2.data());
	inode_array[i].type = 'd';
	strcpy(inode_array[i].user_name, user.user_name);
	inode_array[i].iparent = cur_loc;
	inode_array[i].length = 0;
	save_inode(i);
}

// 功能: 在当前目录下创建文件(creat file1)
void touch(void)
{
	if (s2.length() == 0) {
		printf("Please input filename.\n");
		return;
	}
	int i, tmp_cur; string tmps1, tmps2;
	if (s2.find('/') != -1) {
		tmps1 = s2.substr(0, s2.find_last_of('/') + 1);
		tmps2 = s2.substr(s2.find_last_of('/') + 1);
		s2 = tmps1;
		tmp_cur = readby();
		if (tmp_cur == -1) {
			printf("No Such Directory\n");
		}
	}
	else {
		tmps2 = s2;
		tmp_cur = cur_loc;
	}
	for (i = 0; i < INODENUM; i++)
		if ((inode_array[i].inum > 0) &&
			(inode_array[i].type == '-') &&
			tmps2 == inode_array[i].file_name &&
			inode_array[i].iparent == tmp_cur &&
			!strcmp(inode_array[i].user_name, user.user_name)) break;
	if (i != INODENUM) {
		printf("There is same file\n");
		return;
	}
	for (i = 0; i < INODENUM; i++)
		if (inode_array[i].inum < 0) break;
	if (i == INODENUM)
	{
		printf("Inode is full.\n");
		exit(-1);
	}
	inode_array[i].inum = i;
	strcpy(inode_array[i].file_name, tmps2.data());
	inode_array[i].type = '-';
	strcpy(inode_array[i].user_name, user.user_name);
	inode_array[i].iparent = tmp_cur;
	inode_array[i].length = 0;
	save_inode(i);
}

//检查当前I节点的文件是否属于当前用户
int check(int i)
{
	int j;
	char* uuser, * fuser;
	uuser = user.user_name;
	fuser = inode_array[i].user_name;
	j = strcmp(fuser, uuser);
	if (j == 0)  return 1;
	else      return 0;
}

void open(int mymode) {
	/*功能: 打开当前目录下的文件(open file1)*/
	int i, inum, mode, filenum, chk;
	if (s2.length() == 0) {
		printf("This file doesn't exist.\n");
		return;
	}
	int tmp_cur; string tmps1, tmps2;
	if (s2.find('/') != -1) {
		tmps1 = s2.substr(0, s2.find_last_of('/') + 1);
		tmps2 = s2.substr(s2.find_last_of('/') + 1);
		string tmps = s2;
		s2 = tmps1;
		tmp_cur = readby();
		s2 = tmps;
		if (tmp_cur == -1) {
			cout << "No Such Directory" << endl;
		}
	}
	else {
		tmps2 = s2;
		tmp_cur = cur_loc;
	}
	for (i = 0; i < INODENUM; i++)
		if ((inode_array[i].inum > 0) &&
			(inode_array[i].type == '-') &&
			tmps2 == inode_array[i].file_name &&
			!strcmp(inode_array[i].user_name, user.user_name) &&
			inode_array[i].iparent == tmp_cur)     //判断是否在当前目录下 
			break;
	if (i == INODENUM) {
		cout << "This is no " + tmps2 + " file...\n";
		return;
	}
	inum = i;
	chk = check(i);               //检查该文件是否为当前用户的文件
	mode = mymode;
	if (chk != 1) {
		printf("This file is not yours !\n");
		return;
	}
	if ((mode < 1) || (mode > 3)) {
		printf("Open mode is wrong.\n");
		return;
	}
	filenum = i;
	file_array[filenum].inum = inum;
	strcpy(file_array[filenum].file_name, inode_array[inum].file_name);
	file_array[filenum].mode = mode;
	file_array[filenum].offset = 0;
}

void close() {
	/*功能: 关闭已经打开的文件(close file1)*/
	int i;
	int tmp_cur; string tmps1, tmps2;
	if (s2.find('/') != -1) {
		tmps1 = s2.substr(0, s2.find_last_of('/') + 1);
		tmps2 = s2.substr(s2.find_last_of('/') + 1);
		string tmps = s2;
		s2 = tmps1;
		tmp_cur = readby();
		s2 = tmps;
	}
	else {
		tmps2 = s2;
		tmp_cur = cur_loc;
	}
	for (i = 0; i < FILENUM; i++)
		if ((file_array[i].inum > 0) &&
			(tmps2 == file_array[i].file_name)) break;
	if (i == FILENUM) {
		printf("This file didn't be opened.\n");
		return;
	}
	else
		file_array[i].inum = -1;
}

void cat() {
	int i, inum;
	open(1);
	string tmps1, tmps2; int tmp_cur;
	if (s2.find('/') != -1) {
		tmps1 = s2.substr(0, s2.find_last_of('/') + 1);
		tmps2 = s2.substr(s2.find_last_of('/') + 1);
		string tmps = s2;
		s2 = tmps1;
		tmp_cur = readby();
		s2 = tmps;
	}
	else {
		tmps2 = s2;
		tmp_cur = cur_loc;
	}
	for (i = 0; i < FILENUM; i++)
		if ((file_array[i].inum > 0) &&
			(tmps2 == file_array[i].file_name))
			break;
	inum = file_array[i].inum;
	if (inode_array[inum].length > 0) {
		read_blk(inum);
		for (i = 0; tmp[i] != '\0'; i++)
			printf("%c", tmp[i]);
		printf("\n");
	}
	close();
}


void vi() {
	/*功能: 向文件中写入字符(write file1)*/
	int i, inum, length;
	open(3);
	for (i = 0; i < FILENUM; i++)
		if ((file_array[i].inum > 0) &&
			s2 == file_array[i].file_name) break;
	if (i == FILENUM) {
		cout << "Open " << s2 << " false.\n";
		return;
	}
	inum = file_array[i].inum;
	//printf("The length of %s:%d\n", inode_array[inum].file_name, inode_array[inum].length);
	if (inode_array[inum].length == 0) {
		printf("The length you want to write(0-1024):");
		scanf("%d", &length);
		gets(tmp);
		if ((length < 0) && (length > 1024)) {
			printf("You can't creat a file which data less than 0 byte or more than 1024 bytes.'.\n");
			return;
		}
		inode_array[inum].length = length;
		inode_array[inum].address[0] = get_blknum();
		if (length > 512)
			inode_array[inum].address[1] = get_blknum();
		save_inode(inum);
		printf("Input the data(Enter to end):\n");
		//gets(tmp);

		char c;
		char m[1024];
		string str;
		while ((c = getchar()) != '#') {
			if (c == 0x0d) { //当输入回车键时，0x0d为回车键的ASCII码
				c = '0x0a'; //将输入的回车键转换成换行符
			}
			gets(m);
			str = str + m + "\n";
		}
		for (int i = 0; i < 1024; i++) {
			tmp[i] = str[i];
		}

		write_blk(inum);
	}
	else {
		printf("Are you sure to write this file? Data in it will all be deleted.(y/n):");
		scanf("%c", &choice);
		if ((choice == 'y') || (choice == 'Y')) {
			printf("The length you want to write(0-1024):");
			scanf("%d", &length);
			gets(tmp);
			if ((length < 0) && (length > 1024)) {
				printf("You can't creat a file which data less than 0 byte or more than 1024 bytes.'.\n");
				return;
			}
			inode_array[inum].length = length;
			inode_array[inum].address[0] = get_blknum();
			if (length > 512)
				inode_array[inum].address[1] = get_blknum();
			save_inode(inum);
			printf("Input the data(Enter to end):\n");
			gets(tmp);
			write_blk(inum);
		}
	}
	close();
	int tmp_cur = cur_loc;
	init();   //需要初始化一下，否则文件有一个文件名会出错 
	cur_loc = tmp_cur;
}

void su() {
	/*功能: 切换当前用户(logout)*/
	char* p;
	int flag;
	string user_name;
	char password[10];
	char file_name[10] = "user.txt";
	fp = fopen(file_name, "r");           //初始化指针，将文件系统的指针指向文件系统的首端(以只读方式打开文件)
										  /*if (argc != 2){
										  printf("command su must have one object. \n");
										  return;
										  }*/
	do {
		user_name = s2;
		printf("password:");
		p = password;
		while (*p = _getch()) {
			if (*p == 0x0d) { 		//当输入回车键时，0x0d为回车键的ASCII码
				*p = '\0'; 			//将输入的回车键转换成空格
				break;
			}
			printf("*");   //将输入的密码以"*"号显示
			p++;
		}
		flag = 0;
		while (!feof(fp)) {
			fread(&user, sizeof(User), 1, fp);
			// 已经存在的用户, 且密码正确
			if ((user.user_name == user_name) &&
				!strcmp(user.password, password)) {
				fclose(fp);
				printf("\n");
				return;     //登陆成功，直接跳出登陆函数 
			}
			// 已经存在的用户, 但密码错误
			else if ((user.user_name == user_name)) {
				printf("\nThis user is exist, but password is incorrect.\n");
				flag = 1;    //设置flag为1，表示密码错误，重新登陆 
				fclose(fp);
				break;
			}
		}
		if (flag == 0) {
			printf("\nThis user is not exist.\n");
			break;     //用户不存在，直接跳出循环，进行下一条指令的输入
		}
	} while (flag);
}

//根据Inode节点号删存储
void delet(int innum)
{

	inode_array[innum].inum = -1;
	if (inode_array[innum].length >= 0)
	{
		release_blk(inode_array[innum].address[0]);
		if (inode_array[innum].length >= 512)
			release_blk(inode_array[innum].address[1]);
	}
	save_inode(innum);
}

// 功能: 删除文件
void rmfile(void)
{
	if (s2.length() == 0) {
		printf("This file doesn't exist.\n");
		return;
	}
	int i, tmp_cur; string tmps1, tmps2;
	if (s2.find('/') != -1) {
		tmps1 = s2.substr(0, s2.find_last_of('/') + 1);
		tmps2 = s2.substr(s2.find_last_of('/') + 1);
		s2 = tmps1;
		tmp_cur = readby();
	}
	else {
		tmps2 = s2;
		tmp_cur = cur_loc;
	}
	for (i = 0; i < INODENUM; i++)
		if ((inode_array[i].inum > 0) &&
			(inode_array[i].type == '-') &&
			tmps2 == inode_array[i].file_name &&
			inode_array[i].iparent == tmp_cur &&
			!strcmp(inode_array[i].user_name, user.user_name)) break;
	if (i == INODENUM)
	{
		printf("This file doesn't exist.\n");
		return;
	}
	delet(i);
}

// 功能: 退出当前用户(logout)
void logout()
{
	login();
}


// 功能: 退出文件系统(quit)
void quit()
{
	printf("Bye~\n");
	exit(0);
}

// 功能: 显示错误
void errcmd()
{
	printf("Command Error!!!\n");
}

//清空内存中存在的用户名
void free_user()
{
	int i;
	for (i = 0; i < 10; i++)
		user.user_name[i] = '\0';
}
// 功能: 循环执行用户输入的命令, 直到logout
// "help", "cd", "dir", "mkdir", "touch", "open","read", "write", "close", "delete", "logout", "clear", "format","quit","rd"

void command(void)
{
	system("cls");
	do
	{
		pathset();
		switch (analyse())
		{
		case -1:
			printf("\n");
			break;
		case 0:
			help();
			break;
		case 1:
			cd();
			break;
		case 2:
			dir();
			break;
		case 3:
			mkdir();
			break;
		case 4:
			touch();
			break;
		case 5:
			//open();
			break;
		case 6:
			cat();
			break;
		case 7:
			vi();
			break;
		case 8:
			//close();
			break;
		case 9:
			rmfile();
			break;
		case 10:
			su();
			break;
		case 11:
			system("cls");
			break;
		case 12:
			format();
			init();
			free_user();
			login();
			break;
		case 13:
			quit();
			break;
		case 14:
			rmdir();
			break;
		case 15:
			df();
			break;
		case 16:
			errcmd();
			break;
		default:
			break;
		}
	} while (1);
}


// 主函数
int main(void)
{
	login();
	init();
	command();
	return 0;
}