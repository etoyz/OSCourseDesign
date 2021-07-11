#include "head.h"

string cmd1, cmd2;
int cur_loc;                       // 当前目录
char tmp[2 * BLKSIZE];               // 缓冲区
User user;                           // 当前的用户
char bitmap[BLKNUM];               // 位图数组
Inode inode_array[INODENUM];       // i节点数组
File_table file_table[FILENUM];       // 打开文件表数组
char image_name[10] = "image.dat"; // 文件系统名称
FILE *fp;                           // 文件指针

const string Commands[] = {"help", "cd", "ls", "mkdir", "touch", "cat", "vi", "rm", "clear", "format", "exit", "rmdir",
                           "df"};

int readby() { //根据当前目录和第二个参数, 返回目标目录的inode号
    if (cmd2[cmd2.length() - 1] != '/')
        cmd2 += '/';
    int result_cur = 0;
    int tmp_cur = cur_loc;
    vector <string> v; // 存储路径
    while (cmd2.find('/') != -1) {
        v.push_back(cmd2.substr(0, cmd2.find_first_of('/')));
        cmd2 = cmd2.substr(cmd2.find_first_of('/') + 1);
    }
    if (v.size() == 0) {
        return cur_loc;
    }
    if (v[0].length() == 0) {
        tmp_cur = 0;
    } else if (v[0] == "..") {
        if (inode_array[cur_loc].inum > 0) {
            tmp_cur = inode_array[cur_loc].iparent;
        }
    } else {
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
        } else {
            tmp_cur = i;
        }
    }
    int i;
    for (unsigned int count = 1; count < v.size(); count++) {
        for (i = 0; i < INODENUM; i++) {
            if ((inode_array[i].inum > 0) && //是否为空
                (inode_array[i].iparent == tmp_cur) &&
                (inode_array[i].type == 'd') &&
                inode_array[i].file_name == v[count]) {
                break;
            }
        }
        if (i == INODENUM) {
            return -1;
        } else {
            tmp_cur = i;
        }
    }
    result_cur = tmp_cur;
    return result_cur;
}

//创建磁盘映像，并将所有用户和文件清除
void format(void) {
    int i;
    printf("Formating filesystem...\n");
    printf("All the old data will lost!!!\n");
    printf("Are you sure format the fileSystem?(Y/N)?");
    char choice;
    cin >> choice;
    if ((choice == 'y') || (choice == 'Y')) {
        // 打开映像文件
        fp = fopen(image_name, "w+b");
        if (fp == NULL) {
            printf("Can't open file %s\n", image_name);
            getchar();
            exit(-1);
        }
        //成功打开后，位图数组置0
        for (i = 0; i < BLKNUM; i++)
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
        if (fp == NULL) {
            printf("Can't create file %s\n", "user.txt");
            exit(-1);
        }
        fclose(fp);
        printf("Filesystem created successful!\n");
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
    int flag = 0;
    char user_name[10];
    char password[10];
    char choice; //选择是否（y/n）
    do {
        // 输入用户名、密码
        printf("Please Login First!\n");
        printf("username:");
        gets(user_name);
        printf("password:");
	gets(password);
        /* windows 下可实现隐藏密码输入
	char *p = password;
        while (*p = getch()) {
            if (*p == 0x0d) {               //当输入回车时
                *p = '\0'; //将输入的回车符转换成字符串结束符
                break;
            }
            printf("*"); //将输入的密码以"*"号显示
            p++;
        }*/
        // 校验用户名、密码
        while (!feof(fp)) {
            fread(&user, sizeof(User), 1, fp);
            // 已经存在的用户, 且密码正确
            if (!strcmp(user.user_name, user_name) &&
                !strcmp(user.password, password)) {
                fclose(fp);
                printf("\n");
                return; //校验成功，直接跳出登陆函数
            }
                // 已经存在的用户, 但密码错误
            else if (!strcmp(user.user_name, user_name)) {
                printf("\nPassword incorrect!\n");
                flag = 1; //密码错误，重新登陆
                fclose(fp);
                break;
            }
                // 不存在的用户名
            else {
                printf("\nThis user doesn't exist.\n");
                break; //用户不存在，先跳出循环创建新用户
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
        } else
            quit();
    }
}

// 功能: 将所有i节点读入内存
void init(void) {
    if ((fp = fopen(image_name, "r+b")) == NULL) {
        printf("Can't open file %s.\n", image_name);
        exit(-1);
    }
    // 读入位图
    for (int i = 0; i < BLKNUM; i++)
        bitmap[i] = fgetc(fp);
    // 显示位图
    // 读入i节点信息
    for (int i = 0; i < INODENUM; i++)
        fread(&inode_array[i], sizeof(Inode), 1, fp);
    // 显示i节点
    // 当前目录为根目录
    cur_loc = 0;
    // 初始化打开文件表
    for (int i = 0; i < FILENUM; i++)
        file_table[i].inum = -1;
}

void df() {
    int cnt_inode = 0, cnt_mem = 0;
    for (int i = 0; i < INODENUM; i++) {
        if (inode_array[i].inum > 0)
            cnt_inode++;
    }
    for (int i = 0; i < BLKNUM; i++) {
        if (bitmap[i] == '1')
            cnt_mem++;
    }
    cout << "Inode use:" << cnt_inode << "/" << INODENUM << endl;
    cout << "Store use:" << cnt_mem << "/" << BLKNUM << endl;
}

// 功能: 将num号i节点保存到hd.dat
void save_inode(int num) {
    if ((fp = fopen(image_name, "r+b")) == NULL) {
        printf("Can't open file %s\n", image_name);
        exit(-1);
    }
    fseek(fp, BLKNUM + num * sizeof(Inode), SEEK_SET);
    fwrite(&inode_array[num], sizeof(Inode), 1, fp);
    fclose(fp);
}

// 功能: 申请一个数据块
int get_blknum(void) {
    int i;
    for (i = 0; i < BLKNUM; i++)
        if (bitmap[i] == '0')
            break;
    // 未找到空闲数据块
    if (i == BLKNUM) {
        printf("Data area is full.\n");
        exit(-1);
    }
    bitmap[i] = '1';
    if ((fp = fopen(image_name, "r+b")) == NULL) {
        printf("Can't open file %s\n", image_name);
        exit(-1);
    }
    fseek(fp, i, SEEK_SET);
    fputc('1', fp);
    fclose(fp);
    return i;
}

// 功能: 将i节点号为num的文件读入tmp
void read_blk(int num) {
    int i, len;
    char ch;
    int add0, add1;
    len = inode_array[num].length;
    add0 = inode_array[num].address[0];
    if (len > BLKSIZE)
        add1 = inode_array[num].address[1];
    if ((fp = fopen(image_name, "r+b")) == NULL) {
        printf("Can't open file %s.\n", image_name);
        exit(-1);
    }
    fseek(fp, BLKOFFSET + add0 * BLKSIZE, SEEK_SET); // 前1536是
    ch = fgetc(fp);
    for (i = 0; (i < len) && (ch != '\0') && (i < BLKSIZE); i++) {
        tmp[i] = ch;
        ch = fgetc(fp);
    }
    if (i >= BLKSIZE) {
        fseek(fp, BLKOFFSET + add1 * BLKSIZE, SEEK_SET);
        ch = fgetc(fp);
        for (; (i < len) && (ch != '\0'); i++) {
            tmp[i] = ch;
            ch = fgetc(fp);
        }
    }
    tmp[i] = '\0';
    fclose(fp);
}

// 功能: 将tmp的内容输入hd的数据区
void write_blk(int num) {
    int i, len;
    int add0, add1;
    add0 = inode_array[num].address[0];
    len = inode_array[num].length;
    if ((fp = fopen(image_name, "r+b")) == NULL) {
        printf("Can't open file %s.\n", image_name);
        exit(-1);
    }
    fseek(fp, BLKOFFSET + add0 * BLKSIZE, SEEK_SET);
    for (i = 0; (i < len) && (tmp[i] != '\0') && (i < BLKSIZE); i++)
        fputc(tmp[i], fp);
    if (i == BLKSIZE) {
        add1 = inode_array[num].address[1];
        fseek(fp, BLKOFFSET + add1 * BLKSIZE, SEEK_SET);
        for (; (i < len) && (tmp[i] != '\0'); i++)
            fputc(tmp[i], fp);
    }
    fputc('\0', fp);
    fclose(fp);
}

// 功能: 释放文件块号为num的文件占用的空间
void release_blk(int num) {
    FILE *fp;
    if ((fp = fopen(image_name, "r+b")) == NULL) {
        printf("Can't open file %s\n", image_name);
        exit(-1);
    }
    bitmap[num] = '0';
    fseek(fp, num, SEEK_SET);
    fputc('0', fp);
    fclose(fp);
}

//设置文件路径
void pathset() {
    string s;
    if (inode_array[cur_loc].inum == 0)
        s = "";
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
void cd() {
    int tmp_cur;
    if (cmd2.length() == 0) { // 如果无参数，则去根目录
        tmp_cur = 0;
    } else { // 否则去目标目录
        tmp_cur = readby();
    }
    if (tmp_cur != -1) {
        cur_loc = tmp_cur;
    } else {
        cout << "No Such Directory" << endl;
    }
}

// 功能: 显示当前目录下的子目录和文件(dir)
void dir(void) {
    int tmp_cur;
    int i = 0;
    if (cmd2.length() == 0) {
        tmp_cur = cur_loc;
    } else {
        tmp_cur = readby();
        if (tmp_cur == -1 || inode_array[tmp_cur].type != 'd') {
            cout << "No Such Directory" << endl;
            return;
        }
    }
    for (i = 0; i < INODENUM; i++) {
        if ((inode_array[i].inum > 0) &&                          // 有效节点
            (inode_array[i].iparent == tmp_cur)                      // 当前目录下
            && !strcmp(inode_array[i].user_name, user.user_name)) // 属于当前用户
        {
            if (inode_array[i].type == 'd')
                printf("%s\t\t\t<DIR>\n", inode_array[i].file_name);
            if (inode_array[i].type == '-')
                printf("%s\t\t%12d bytes\n", inode_array[i].file_name, inode_array[i].length);
        }
    }
}

void rm(int inode) {
    int i;
    for (i = 0; i < INODENUM; i++) {
        if (inode_array[i].iparent == inode && !strcmp(inode_array[i].user_name, user.user_name)) {
            if (inode_array[i].type == 'd')
                rm(i);
            else
                delet(i);
        }
    }
    delet(inode);
}

void rmdir() {
    int tmp_cur = readby();
    if (tmp_cur == -1 || tmp_cur == cur_loc)
        printf("No Such Directory\n");
    else {
        if (inode_array[tmp_cur].type != 'd') {
            printf("That's A File! (Try rm command ?)\n");
        } else {
            int tmp = inode_array[cur_loc].iparent;
            while (tmp != 0) { // 检测要删除的是否为父级目录，若是，则不能删除
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
void mkdir(void) {
    int i;
    if (cmd2.length() == 0) {
        cout << "Please input name" << endl;
        return;
    }
    for (i = 0; i < INODENUM; i++) { // 遍历i节点数组, 查找未用的i节点
        if (inode_array[i].iparent == cur_loc && inode_array[i].type == 'd' && inode_array[i].file_name == cmd2 &&
            inode_array[i].inum > 0 && !strcmp(inode_array[i].user_name, user.user_name)) {
            break;
        }
    }
    if (i != INODENUM) {
        printf("There is directory having same name.\n");
        return;
    }
    for (i = 0; i < INODENUM; i++)
        if (inode_array[i].inum < 0)
            break;
    if (i == INODENUM) {
        printf("Inode is full.\n");
        exit(-1);
    }
    inode_array[i].inum = i;
    strcpy(inode_array[i].file_name, cmd2.data());
    inode_array[i].type = 'd';
    strcpy(inode_array[i].user_name, user.user_name);
    inode_array[i].iparent = cur_loc;
    inode_array[i].length = 0;
    save_inode(i);
}

// 功能: 在指定目录下创建空文件
void touch(void) {
    if (cmd2.length() == 0) {
        printf("Please input filename！\n");
        return;
    }
    int i, tmp_cur;
    string fileLoc, fileName;
    if (cmd2.find('/') != -1) {
        fileLoc = cmd2.substr(0, cmd2.find_last_of('/') + 1);
        fileName = cmd2.substr(cmd2.find_last_of('/') + 1);
        cmd2 = fileLoc;
        tmp_cur = readby();
        if (tmp_cur == -1) {
            printf("No Such Directory\n");
        }
    } else {
        fileName = cmd2;
        tmp_cur = cur_loc;
    }
    // 检测文件名是否存在
    for (i = 0; i < INODENUM; i++)
        if ((inode_array[i].inum > 0) &&                       // 有效节点
            (inode_array[i].type == '-') &&                       // 文件类型
            fileName == inode_array[i].file_name &&               // 文件名相同
            inode_array[i].iparent == tmp_cur &&               // 目录符合
            !strcmp(inode_array[i].user_name, user.user_name)) // 属于当前用户
            break;
    if (i != INODENUM) {
        printf("The file name already exists\n");
        return;
    }
    // 检测Inode是否已经存满
    for (i = 0; i < INODENUM; i++)
        if (inode_array[i].inum < 0)
            break;
    if (i == INODENUM) {
        printf("Inode is full！\n");
        exit(-1);
    }
    // 创建新的空文件
    inode_array[i].inum = i;
    strcpy(inode_array[i].file_name, fileName.data());
    inode_array[i].type = '-';
    strcpy(inode_array[i].user_name, user.user_name);
    inode_array[i].iparent = tmp_cur;
    inode_array[i].length = 0; // 文件初始大小为0
    save_inode(i);
}

//检查当前I节点的文件是否属于当前用户
int check(int i) {
    int j;
    char *uuser, *fuser;
    uuser = user.user_name;
    fuser = inode_array[i].user_name;
    j = strcmp(fuser, uuser);
    if (j == 0)
        return 1;
    else
        return 0;
}

/* 功能: 打开当前目录下的文件； 目前mode仅作为一个字段，无其他含义 */
int open(int mode) {
    int i, inum, filenum, chk;
    if (cmd2.length() == 0) {
        printf("This file doesn't exist.\n");
        return -1;
    }
    int tmp_cur;
    string fileLoc, fileName;
    if (cmd2.find('/') != -1) {
        fileLoc = cmd2.substr(0, cmd2.find_last_of('/') + 1);
        fileName = cmd2.substr(cmd2.find_last_of('/') + 1);
        string tmps = cmd2;
        cmd2 = fileLoc;
        tmp_cur = readby();
        cmd2 = tmps;
        if (tmp_cur == -1) {
            cout << "No Such Directory" << endl;
        }
    } else {
        fileName = cmd2;
        tmp_cur = cur_loc;
    }
    for (i = 0; i < INODENUM; i++)
        if ((inode_array[i].inum > 0) &&
            (inode_array[i].type == '-') &&
            fileName == inode_array[i].file_name &&
            !strcmp(inode_array[i].user_name, user.user_name) &&
            inode_array[i].iparent == tmp_cur) //判断是否在当前目录下
            break;
    if (i == INODENUM) {
        cout << "This is no \"" + fileName + "\" file...\n";
        return -1;
    }
    inum = i;
    chk = check(i); //检查该文件是否为当前用户的文件
    if (chk != 1) {
        printf("This file is not yours !\n");
        return -1;
    }
    if ((mode < 1) || (mode > 3)) {
        printf("Open mode is wrong.\n");
        return -1;
    }
    filenum = i;
    file_table[filenum].inum = inum;
    strcpy(file_table[filenum].file_name, inode_array[inum].file_name);
    file_table[filenum].mode = mode;
    file_table[filenum].offset = 0;
    return 0;
}

/*功能: 关闭已经打开的文件 */
void close() {
    int i;
    int tmp_cur;
    string fileLoc, fileName;
    if (cmd2.find('/') != -1) {
        fileLoc = cmd2.substr(0, cmd2.find_last_of('/') + 1);
        fileName = cmd2.substr(cmd2.find_last_of('/') + 1);
        string tmps = cmd2;
        cmd2 = fileLoc;
        tmp_cur = readby();
        cmd2 = tmps;
    } else {
        fileName = cmd2;
        tmp_cur = cur_loc;
    }
    for (i = 0; i < FILENUM; i++)
        if ((file_table[i].inum > 0) &&
            (fileName == file_table[i].file_name))
            break;
    if (i == FILENUM) {
        printf("This file didn't be opened.\n");
        return;
    } else
        file_table[i].inum = -1;
}

void cat() {
    if(open(1) != 0) {
        cout << "error occurs！" << endl;
        return;
    }
    string fileLoc, fileName;
    int tmp_cur;
    if (cmd2.find('/') != -1) {
        fileLoc = cmd2.substr(0, cmd2.find_last_of('/') + 1);
        fileName = cmd2.substr(cmd2.find_last_of('/') + 1);
        string tmps = cmd2;
        cmd2 = fileLoc;
        tmp_cur = readby();
        cmd2 = tmps;
    } else {
        fileName = cmd2;
        tmp_cur = cur_loc;
    }
    int i;
    for (i = 0; i < FILENUM; i++)
        if ((file_table[i].inum > 0) &&
            (fileName == file_table[i].file_name))
            break;
    int inum = file_table[i].inum;
    if (inode_array[inum].length > 0) {
        read_blk(inum);
        for (i = 0; tmp[i] != '\0'; i++)
            printf("%c", tmp[i]);
        printf("\n");
    }
    close();
}

/*功能: 向文件中写入字符 */
void vi() {
    if(open(3) != 0) {
        cout << "error occurs！" << endl;
        return;
    }
    int i, inum;
    for (i = 0; i < FILENUM; i++)
        if ((file_table[i].inum > 0) &&
            cmd2 == file_table[i].file_name)
            break;
    if (i == FILENUM) {
        cout << "Open " << cmd2 << " false.\n";
        return;
    }
    inum = file_table[i].inum;
    if (inode_array[inum].length == 0) {
        printf("Input the data(Enter to end):\n");
        gets(tmp);
        if (strlen(tmp) > BLKSIZE * 2) {
            printf("You can't creat a file which data more than %d bytes.'.\n", BLKSIZE * 2);
            return;
        }
        inode_array[inum].length = strlen(tmp);
        inode_array[inum].address[0] = get_blknum();
        if (strlen(tmp) > BLKSIZE)
            inode_array[inum].address[1] = get_blknum();
        save_inode(inum);
        write_blk(inum);
    } else {
        printf("Are you sure to write this file? Data in it will all be deleted.(y/n):");
        char choice;
        scanf("%c", &choice);
        if ((choice == 'y') || (choice == 'Y')) {
            printf("Input the data(Enter to end):\n");
            gets(tmp);
            if (strlen(tmp) > BLKSIZE * 2) {
                printf("You can't creat a file which data more than %d bytes.'.\n", BLKSIZE * 2);
                return;
            }
            inode_array[inum].length = strlen(tmp);
            inode_array[inum].address[0] = get_blknum();
            if (strlen(tmp) > BLKSIZE)
                inode_array[inum].address[1] = get_blknum();
            save_inode(inum);
            write_blk(inum);
        }
    }
    close();
    int tmp_cur = cur_loc;
    init(); //需要初始化一下，否则文件有一个文件名会出错
    cur_loc = tmp_cur;
}

//根据Inode节点号删存储
void delet(int innum) {
    inode_array[innum].inum = -1;
    if (inode_array[innum].length >= 0) {
        release_blk(inode_array[innum].address[0]);
        if (inode_array[innum].length >= BLKSIZE)
            release_blk(inode_array[innum].address[1]);
    }
    save_inode(innum);
}

void rmfile(void) {
    if (cmd2.length() == 0) {
        printf("This file doesn't exist.\n");
        return;
    }
    int i, tmp_cur;
    string fileLoc, fileName;
    if (cmd2.find('/') != -1) {
        fileLoc = cmd2.substr(0, cmd2.find_last_of('/') + 1);
        fileName = cmd2.substr(cmd2.find_last_of('/') + 1);
        cmd2 = fileLoc;
        tmp_cur = readby();
    } else {
        fileName = cmd2;
        tmp_cur = cur_loc;
    }
    for (i = 0; i < INODENUM; i++)
        if ((inode_array[i].inum > 0) &&
            (inode_array[i].type == '-') &&                       //文件类型
            fileName == inode_array[i].file_name &&               //文件名符合
            inode_array[i].iparent == tmp_cur &&               // 父目录符合
            !strcmp(inode_array[i].user_name, user.user_name)) // 权限检查
            break;
    if (i == INODENUM) {
        printf("This file doesn't exist.\n");
        return;
    }
    delet(i);
}

// 功能: 退出文件系统(quit)
void quit() {
    printf("Bye~\n");
    exit(0);
}

void help() {
    printf("command: \n\
	help   ---  show help menu \n\
	clear  ---  clear the screen \n\
	vi     ---  open and write something to a particular file \n\
	rm     ---  delete a exist file \n\
	ls     ---  show all the files and directories in particular directory \n\
	cd     ---  change directory \n\
	mkdir  ---  make a new directory   \n\
	rmdir  ---  delete a directory \n\
	touch  ---  create a new file \n\
	cat    ---  open and read an exist file \n\
	format ---  format a exist filesystem \n\
	df     ---  show the useage of storage \n\
	exit   ---  exit this system \n");
}

// 结果: 0-12为系统命令, 13为命令错误
int cmd_analyze() {
    string s = "";
    getline(cin, s);
    if (s.find(' ') == -1) { // 如果不含空格，则是无参数的命令
        cmd1 = s;
        cmd2 = "";
    } else { //否则是有参数的命令
        cmd1 = s.substr(0, s.find_first_of(' '));
        cmd2 = s.substr(s.find_first_of(' ') + 1);
    }
    int choice;
    for (choice = 0; choice < 13; choice++) {
        if (cmd1 == Commands[choice])
            break;
	}
    return choice;
}

// 功能: 循环执行用户输入的命令
void command(void) {
    //system("cls");
    system("clear");
    printf("Login success!\n");
    printf("Type \"help\" for more info.\n\n");
    do {
        pathset();
        switch (cmd_analyze()) {
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
                cat();
                break;
            case 6:
                vi();
                break;
            case 7:
                rmfile();
                break;
            case 8:
                system("clear");
                break;
            case 9:
                format();
                printf("Format success, please reopen the program!\n");
                quit();
            case 10:
                quit();
                break;
            case 11:
                rmdir();
                break;
            case 12:
                df();
                break;
            case 13:
                printf("Command Error!!!\n");
                break;
            default:
                break;
        }
    } while (1);
}

// 主函数
int main(void) {
    login();
    init();
    command();
    return 0;
}

