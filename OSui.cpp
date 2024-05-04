/***
__          __  _ _ _                    ____   _____
\ \        / / (_) (_)                  / __ \ / ____|
\ \  /\  / /__ _| |_  __ _ _ __   __ _| |  | | (___
 \ \/  \/ / _ \ | | |/ _` | '_ \ / _` | |  | |\___ \
  \  /\  /  __/ | | | (_| | | | | (_| | |__| |____) |
   \/  \/ \___|_|_|_|\__,_|_| |_|\__, |\____/|_____/
                                  __/ |
                                 |___/
***/

#include "OSui.h"
#include "ToolFun.h"
#include <iostream>

OSui::OSui()
{
  std::cout << "正在初始化命令行界面......\n";
  initial();
  std::cout << "命令行界面初始化完成......\n";
}

OSui::~OSui() {}

void OSui::initial()
{
  DWORD size = sizeof(computerName) / sizeof(computerName[0]);
  GetComputerName(computerName, &size);
  this->is_login = false;
  this->user = "";
  this->group = "";
  this->uid = -1;
  this->gid = -1;
  this->cur_dir = "";
}

void OSui::format()
{
  std::cout << "确定格式化文件系统？[y/n]: ";
  std::string s;
  std::cin >> s;
  if (s == "y" || "Y")
  {
    std::cout << "正在格式化文件系统.......\n";
    FM.fformat();
    initial();
    std::cout << "文件系统格式化完成.......\n";
    system("pause");
    cls();
  }
  else
    return;
}

void OSui::mkdir(const std::string &path)
{
  std::string new_dir_path = findPath(path, cur_dir);
  FM.dcreate(new_dir_path, uid, gid);
}

void OSui::cd(const std::string &path)
{
  std::string dir_path = findPath(path, cur_dir);
  if (FM.Enter(dir_path, uid, gid))
    cur_dir = dir_path;
  else
    std::cout << "无法进入指定目录\n";
}

void OSui::ls()
{
  FM.LS(cur_dir, uid, gid);
}

void OSui::rm(const std::string &path)
{
  std::string new_path = findPath(path, cur_dir);
  FM.ddelete(new_path, uid, gid);
}

void OSui::touch(const std::string &path)
{
  std::string new_dir_path = findPath(path, cur_dir);
  FM.fcreate(new_dir_path, uid, gid);
}

void OSui::cat(const std::string &path)
{
  std::string file_path = findPath(path, cur_dir);
  File *fp = FM.fopen(file_path, uid, gid);
  char *data = new char[fp->f_inode->i_size + 1]{0};
  FM.fread(data, fp->f_inode->i_size, fp);
  std::cout << data << '\n';
  FM.fclose(fp);
  delete[] data;
}

void OSui::pwd()
{
  std::cout << cur_dir << '\n';
}

void OSui::chmod(const std::string &path, int mod)
{
  std::string file_path = findPath(path, cur_dir);
  FM.ChMod(file_path, mod, uid, gid);
}

void OSui::login(const std::string &uname, const std::string &psw)
{
  File *userf = FM.fopen("/etc/users.txt", 0, 0);
  char *data = new char[userf->f_inode->i_size + 1]{0};
  FM.fread(data, userf->f_inode->i_size, userf);
  std::string sdata = data;
  delete[] data;
  FM.fclose(userf);
  std::vector<std::string> users;
  users = splitstring(sdata, "\n");
  for (unsigned int i = 0; i < users.size(); i++)
  {
    std::vector<std::string> umsg = splitstring(users[i], "-");
    if (umsg[0] == uname && umsg[1] == psw)
    {
      is_login = true;
      uid = stoi(umsg[2]);
      user = uname;
      gid = stoi(umsg[3]);

      File *groupf = FM.fopen("/etc/groups.txt", 0, 0);
      char *data = new char[groupf->f_inode->i_size + 1]{0};
      FM.fread(data, groupf->f_inode->i_size, groupf);
      std::string sdata = data;
      delete[] data;
      FM.fclose(groupf);
      std::vector<std::string> groups;
      groups = splitstring(sdata, "\n");
      for (unsigned int j = 0; j < groups.size(); j++)
      {
        std::vector<std::string> gmsg = splitstring(groups[j], "-");
        if (gmsg[1] == umsg[3])
        {
          group = gmsg[0].c_str();
          break;
        }
      }

      cur_dir = "/";
      break;
    }
  }
}

void OSui::logout()
{
  initial();
}

/*添加用户，组名默认为用户名*/
void OSui::adduser(const std::string &uname, const std::string &pwd, std::string gname)
{
  if (uid != 0)
  {
    std::cout << "权限不足：缺少添加用户权限";
    return;
  }

  if (gname == "")
    gname = uname;

  File *userf = FM.fopen("/etc/users.txt", 0, 0);
  char *data = new char[userf->f_inode->i_size + 1]{0};
  FM.fread(data, userf->f_inode->i_size, userf);
  std::string usdata = data;
  delete[] data;
  std::vector<std::string> users;
  users = splitstring(usdata, "\n");

  int new_uid = 0;
  for (int i = 0; i < users.size(); i++)
  {
    std::vector<std::string> umsg = splitstring(users[i], "-");
    if (umsg[0] == uname)
    {
      FM.fclose(userf);
      std::cout << "该用户已存在\n";
      return;
    }
    if (stoi(umsg[2]) >= new_uid)
      new_uid = stoi(umsg[2]) + 1;
  }

  int new_gid = -1;
  File *groupf = FM.fopen("/etc/groups.txt", 0, 0);
  data = new char[groupf->f_inode->i_size + 1]{0};
  FM.fread(data, groupf->f_inode->i_size, groupf);
  std::string gsdata = data;
  delete[] data;
  std::vector<std::string> groups;
  groups = splitstring(gsdata, "\n");
  for (int i = 0; i < groups.size(); i++)
  {
    std::vector<std::string> gmsg = splitstring(groups[i], "-");
    if (gmsg[0] == gname)
    {
      new_gid = stoi(gmsg[1]);
      if (gmsg.size() != 2)
        groups[i] += ',';
      groups[i] += std::to_string(new_uid);
      break;
    }
  }

  // 用户组不存在，则新建
  if (new_gid == -1)
  {
    FM.fclose(groupf);
    addgroup(gname);
    groupf = FM.fopen("/etc/groups.txt", 0, 0);
    data = new char[groupf->f_inode->i_size + 1]{0};
    FM.fread(data, groupf->f_inode->i_size, groupf);
    gsdata = data;
    delete[] data;
    groups = splitstring(gsdata, "\n");
    std::vector<std::string> gmsg = splitstring(groups[groups.size() - 1], "-");
    new_gid = stoi(gmsg[1]);
    groups[groups.size() - 1] += std::to_string(new_uid);
  }

  std::string new_gdata;
  for (int i = 0; i < groups.size(); i++)
    new_gdata += groups[i] + '\n';
  FM.flseek(groupf, 0, SEEK_SET);
  FM.fwrite(new_gdata.c_str(), new_gdata.length(), groupf);

  usdata += uname + "-" + pwd + "-" + std::to_string(new_uid) + "-" + std::to_string(new_gid) + "\n";
  FM.flseek(userf, 0, SEEK_SET);
  FM.fwrite(usdata.c_str(), usdata.length(), userf);

  FM.fclose(userf);
  FM.fclose(groupf);
}

void OSui::deluser(const std::string &uname)
{
  if (uid != 0)
  {
    std::cout << "权限不足：缺少删除用户权限\n";
    return;
  }

  if (uname == "root")
  {
    std::cout << "权限不足：不能删除root用户\n";
    return;
  }

  File *userf = FM.fopen("/etc/users.txt", 0, 0);
  char *data = new char[userf->f_inode->i_size + 1]{0};
  FM.fread(data, userf->f_inode->i_size, userf);
  std::string sdata = data;
  delete[] data;
  std::vector<std::string> users;
  users = splitstring(sdata, "\n");

  std::string new_udata;
  int d_uid = -1, d_gid = -1;
  for (int i = 0; i < users.size(); i++)
  {
    std::vector<std::string> umsg = splitstring(users[i], "-");
    if (umsg[0] == uname)
    {
      d_uid = stoi(umsg[2]);
      d_gid = stoi(umsg[3]);
    }
    else
      new_udata += users[i] + '\n';
  }

  if (d_uid == -1)
  {
    FM.fclose(userf);
    std::cout << "该用户不存在\n";
    return;
  }

  FM.flseek(userf, 0, SEEK_SET);
  FM.freplace(new_udata.c_str(), new_udata.length(), userf);
  FM.fclose(userf);

  File *groupf = FM.fopen("/etc/groups.txt", 0, 0);
  data = new char[groupf->f_inode->i_size + 1]{0};
  FM.fread(data, groupf->f_inode->i_size, groupf);
  sdata = data;
  delete[] data;
  std::vector<std::string> groups;
  groups = splitstring(sdata, "\n");
  std::string new_gdata;
  for (int i = 0; i < groups.size(); i++)
  {
    std::vector<std::string> gmsg = splitstring(groups[i], "-");
    if (stoi(gmsg[1]) == d_gid)
    {
      std::vector<std::string> uids = splitstring(gmsg[2], "-");
      std::string new_uids;
      for (int j = 0; j < uids.size(); j++)
      {
        if (stoi(uids[j]) != d_uid)
          new_uids += (new_uids.length() == 0 ? uids[j] : "," + uids[j]);
      }
      new_gdata += gmsg[0] + "-" + gmsg[1] + "-" + new_uids + "\n";
    }
    else
      new_gdata += groups[i] + '\n';
  }

  FM.flseek(groupf, 0, SEEK_SET);
  FM.freplace(new_gdata.c_str(), new_gdata.length(), groupf);
  FM.fclose(groupf);
}

void OSui::addgroup(const std::string &gname)
{
  if (uid != 0)
  {
    std::cout << "权限不足：缺少添加用户组权限\n";
    return;
  }

  File *groupf = FM.fopen("/etc/groups.txt", 0, 0);
  char *data = new char[groupf->f_inode->i_size + 1]{0};
  FM.fread(data, groupf->f_inode->i_size, groupf);
  std::string sdata = data;
  delete[] data;
  std::vector<std::string> groups;
  groups = splitstring(sdata, "\n");
  int new_gid = 0;
  for (int j = 0; j < groups.size(); j++)
  {
    std::vector<std::string> gmsg = splitstring(groups[j], "-");
    if (gmsg[0] == gname)
    {
      FM.fclose(groupf);
      std::cout << "该用户组已存在\n";
      return;
    }
    if (stoi(gmsg[1]) >= new_gid)
      new_gid = stoi(gmsg[1]) + 1;
  }
  sdata += gname + '-' + std::to_string(new_gid) + '-' + '\n';
  // std::cout << sdata;
  // groupf->f_offset = 0; // 可以用flseek代替
  FM.flseek(groupf, 0, SEEK_SET);
  FM.fwrite(sdata.c_str(), sdata.length(), groupf);
  FM.fclose(groupf);
}

/*危险操作，删除一个用户组会使其中的用户一并删除，谨慎使用*/
void OSui::delgroup(const std::string &gname)
{
  if (uid != 0)
  {
    std::cout << "权限不足：缺少删除用户组权限\n";
    return;
  }

  if (gname == "root")
  {
    std::cout << "权限不足：不能删除root用户组\n";
    return;
  }

  File *groupf = FM.fopen("/etc/groups.txt", 0, 0);
  char *data = new char[groupf->f_inode->i_size + 1]{0};
  FM.fread(data, groupf->f_inode->i_size, groupf);
  std::string sdata = data;
  delete[] data;
  std::vector<std::string> groups;
  groups = splitstring(sdata, "\n");
  std::string new_gdata;
  std::vector<std::string> uids;
  int tag = false;
  for (int i = 0; i < groups.size(); i++)
  {
    std::vector<std::string> gmsg = splitstring(groups[i], "-");
    if (gmsg[0] == gname)
    {
      tag = true;
      if (gmsg.size() > 2)
        uids = splitstring(gmsg[2], ",");
    }
    else
      new_gdata += groups[i] + '\n';
  }

  if (!tag)
  {
    FM.fclose(groupf);
    std::cout << "该用户组不存在\n";
    return;
  }

  FM.flseek(groupf, 0, SEEK_SET);
  FM.freplace(new_gdata.c_str(), new_gdata.length(), groupf);
  FM.fclose(groupf);

  File *userf = FM.fopen("/etc/users.txt", 0, 0);
  data = new char[userf->f_inode->i_size + 1]{0};
  FM.fread(data, userf->f_inode->i_size, userf);
  sdata = data;
  delete[] data;
  std::vector<std::string> users;
  users = splitstring(sdata, "\n");
  std::string new_udata;
  for (int i = 0; i < users.size(); i++)
  {
    std::vector<std::string> umsg = splitstring(users[i], "-");
    int tag = false;
    for (auto uid : uids)
    {
      if (uid == umsg[2])
      {
        tag = true;
        break;
      }
    }
    if (!tag)
      new_udata += users[i] + '\n';
  }
  FM.flseek(userf, 0, SEEK_SET);
  FM.freplace(new_udata.c_str(), new_udata.length(), userf);
  FM.fclose(userf);
}

void OSui::df()
{
  SuperBlock tmp_spb = FM.m_FileSystem.spb;
  int used = (tmp_spb.s_bsize - tmp_spb.s_fblock) * BLOCK_SIZE;
  int avaial = tmp_spb.s_fblock * BLOCK_SIZE;
  double use_per = 100 * double((double)tmp_spb.s_bsize - (double)tmp_spb.s_fblock) / double(tmp_spb.s_bsize);

  printf("%-10s    %-10s    %-10s    %-10s     %-10s        %s\n", "Filesystem", "Size", "Used", "Avaialable", "Use%", "Monted on");
  printf("%-10s    %-10d    %-10d    %-10d     %-10.2f      %s\n", "WeiliangOS", WLOS_DISK_SIZE, used, avaial, use_per, "/");
}

void OSui::vim(const std::string &path) {}

void OSui::write(const std::string &path, const std::string &content, int pos)
{
  std::string file_path = findPath(path, cur_dir);
  File *fp = FM.fopen(file_path, uid, gid);
  FM.flseek(fp, pos, SEEK_SET);
  FM.fwrite(content.c_str(), content.length(), fp);
  FM.fclose(fp);
}

void OSui::win2wlos(const std::string &fname1, const std::string &fname2)
{
  FILE *winfp;
  fopen_s(&winfp, fname1.c_str(), "rb");
  if (winfp == NULL)
  {
    std::cout << "打开windows文件失败\n";
    return;
  }

  fseek(winfp, 0, SEEK_END);
  unsigned int file_size = ftell(winfp);
  fseek(winfp, 0, SEEK_SET);
  char *data = new char[file_size + 1]{0};
  fread(data, file_size, 1, winfp);
  fclose(winfp);

  std::string file_path;
  if (fname2[0] == '/')
    file_path = fname2;
  else
    file_path = cur_dir + '/' + fname2;
  File *fsfp = FM.fopen(file_path, uid, gid);
  FM.freplace(data, file_size, fsfp);
  FM.fclose(fsfp);
  delete[] data;
}

void OSui::wlos2win(const std::string &fname1, const std::string &fname2)
{
  std::string file_path;
  if (fname1[0] == '/')
    file_path = fname1;
  else
    file_path = cur_dir + '/' + fname1;
  File *fsfp = FM.fopen(file_path, uid, gid);
  int f_size = fsfp->f_inode->i_size;
  char *data = new char[f_size + 1]{0};
  FM.fread(data, f_size, fsfp);
  FM.fclose(fsfp);

  FILE *winfp;
  fopen_s(&winfp, fname2.c_str(), "wb");
  if (winfp == NULL)
  {
    std::cout << "打开windows文件失败\n";
    return;
  }
  fwrite(data, f_size, 1, winfp);
  fclose(winfp);
  delete[] data;
}

void OSui::cls()
{
  system("cls");
}

void OSui::help()
{
  std::cout << "format                                      - 格式化文件系统\n";
  std::cout << "mkdir      <dir name>                       - 创建目录\n";
  std::cout << "cd         <dir name>                       - 进入目录\n";
  std::cout << "ls                                          - 显示当前目录清单\n";
  std::cout << "rm         <file name/dir name>             - 删除文件或文件夹\n";
  std::cout << "touch      <file name>                      - 创建新文件\n";
  std::cout << "cat        <file name>                      - 打印文件内容\n";
  std::cout << "pwd                                         - 打印当前路径\n";
  std::cout << "chmod      <file name/dir name> <mode(OTC>  - 修改文件或目录权限\n";
  std::cout << "login      <user name>                      - 用户登录\n";
  std::cout << "logout                                      - 用户注销\n";
  std::cout << "adduser    <user name> <group name>         - 添加用户\n";
  std::cout << "deluser    <user name>                      - 删除用户\n";
  std::cout << "addgroup   <group name>                     - 添加用户组\n";
  std::cout << "delgroup   <group name>                     - 删除用户组\n";
  std::cout << "su         <user name>                      - 切换用户\n";
  std::cout << "df                                          - 查看磁盘使用情况\n";
  std::cout << "vim        <file name>                      - 用编辑器打开文件\n";
  std::cout << "write      <file name> <pos>                - 写入文件\n";
  std::cout << "win2wlos   <win file name> <fs file name>   - 将Windows文件内容复制到WeliliangOS文件系统文件\n";
  std::cout << "wlos2win   <fs file name> <win file name>   - 将WeliliangOS文件系统文件内容复制到Windows文件\n";
  std::cout << "help                                        - 显示帮助\n";
  std::cout << "cls                                         - 清屏\n";
  std::cout << "exit                                        - 退出系统\n";
}

void OSui::RunOS()
{
  std::cout << " __          __  _ _ _                    ____   _____ \n";
  std::cout << " \\ \\        / / (_) (_)                  / __ \\ / ____|\n";
  std::cout << "  \\ \\  /\\  / /__ _| |_  __ _ _ __   __ _| |  | | (___  \n";
  std::cout << "   \\ \\/  \\/ / _ \\ | | |/ _` | '_ \\ / _` | |  | |\\___ \\ \n";
  std::cout << "    \\  /\\  /  __/ | | | (_| | | | | (_| | |__| |____) |\n";
  std::cout << "     \\/  \\/ \\___|_|_|_|\\__,_|_| |_|\\__, |\\____/|_____/ \n";
  std::cout << "                                    __/ |            \n";
  std::cout << "                                   |___/\n";
  std::cout << "WeiliangOS System\n";
  std::cout << "version : 1.0.0\n";
  std::cout << "Copyright 2024\n";
  std::cout << "ALL Rights Reserved\n";

  while (1)
  {
    std::cout << user << "@";
    std::wcout << computerName << ":" << (user == "root" ? "# " : "$ ");
    std::string input;
    getline(std::cin, input);
    std::vector<std::string> args = splitstring(input, " ");

    if (is_login)
    {
      if (args[0] == "login")
      {
        std::cout << "已经登陆了哦！使用 su 命令切换用户或使用 logout 命令退出登录\n";
      }
      else if (args[0] == "logout")
      {
        std::cout << "退出登录\n";
        logout();
      }
      else if (args[0] == "help")
      {
        help();
      }
      else if (args[0] == "format")
      {
        format();
      }
      else if (args[0] == "mkdir")
      {
        if (argsCheck("mkdir", args.size(), 2))
          mkdir(args[1]);
      }
      else if (args[0] == "cd")
      {
        if (argsCheck("cd", args.size(), 2))
          cd(args[1]);
      }
      else if (args[0] == "ls")
      {
        ls();
      }
      else if (args[0] == "rm")
      {
        if (argsCheck("rm", args.size(), 2))
          rm(args[1]);
      }
      else if (args[0] == "touch")
      {
        if (argsCheck("touch", args.size(), 2))
          touch(args[1]);
      }
      else if (args[0] == "cat")
      {
        if (argsCheck("cat", args.size(), 2))
          cat(args[1]);
      }
      else if (args[0] == "pwd")
      {
        pwd();
      }
      else if (args[0] == "chmod")
      {
        if (argsCheck("chmod", args.size(), 3))
        {
          if (args[2].size() != 3)
          {
            std::cout << " chmod 命令 mode格式错误\n";
            continue;
          }
          int master = args[2][0] - '0';
          int group = args[2][1] - '0';
          int others = args[2][2] - '0';
          if (master >= 0 && master <= 7 && group >= 0 && group <= 7 && others >= 0 && others <= 7)
          {
            int mode = (master << 6) + (group << 3) + others;
            chmod(args[1], mode);
          }
          else
            std::cout << " chmod 命令 mode格式错误\n";
        }
      }
      else if (args[0] == "adduser")
      {
        if (args.size() < 2)
          std::cout << " adduser 命令 参数太少\n";
        else if (args.size() > 3)
          std::cout << " adduser 命令 参数太多\n";
        else
        {
          char password[256] = {0};
          inputPwd(password);
          if (args.size() == 2)
            adduser(args[1], std::string(password));
          else
            adduser(args[1], std::string(password), args[2]);
        }
      }
      else if (args[0] == "deluser")
      {
        if (argsCheck("deluser", args.size(), 2))
          deluser(args[1]);
      }
      else if (args[0] == "addgroup")
      {
        if (argsCheck("addgroup", args.size(), 2))
          addgroup(args[1]);
      }
      else if (args[0] == "delgroup")
      {
        if (argsCheck("delgroup", args.size(), 2))
          delgroup(args[1]);
      }
      else if (args[0] == "su")
      {
        if (argsCheck("su", args.size(), 2))
        {
          char password[256] = {0};
          inputPwd(password);
          login(args[1], std::string(password));
        }
      }
      else if (args[0] == "df")
      {
        df();
      }
      else if (args[0] == "vim")
      {
      }
      else if (args[0] == "write")
      {
        if (argsCheck("write", args.size(), 3))
        {
          std::cout << "请输入写入的内容(在单独一行输入 :q 结束): \n";
          std::string content;
          std::string line;
          while (getline(std::cin, line))
          {
            if (line == ":q")
              break;
            else
              content += line + '\n';
          }
          write(args[1], content, stoi(args[2]));
        }
      }
      else if (args[0] == "win2wlos")
      {
        if (argsCheck("win2wlos", args.size(), 3))
          win2wlos(args[1], args[2]);
      }
      else if (args[0] == "wlos2win")
      {
        if (argsCheck("wlos2win", args.size(), 3))
          wlos2win(args[1], args[2]);
      }
      else if (args[0] == "cls")
      {
        cls();
      }
      else if (args[0] == "exit")
      {
        std::cout << "Bye\n";
        return;
      }
      else
      {
        std::cout << "没有这个指令哦！使用 help 命令查看帮助\n";
      }
    }
    else
    {
      if (args[0] == "login")
      {
        if (argsCheck("login", args.size(), 2))
        {
          char password[256] = {0};
          inputPwd(password);
          login(args[1], std::string(password));
        }
      }
      else if (args[0] == "help")
      {
        help();
      }
      else if (args[0] == "cls")
      {
        cls();
      }
      else if (args[0] == "exit")
      {
        std::cout << "Bye\n";
        return;
      }
      else
      {
        std::cout << "没有这个指令哦！使用 help 命令查看帮助\n";
      }
    }
  }
}

int main()
{
  OSui app;
  app.RunOS();
  return 0;
}
