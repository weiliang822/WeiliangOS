#include <string>
#include <vector>
#include <conio.h>
#include <windows.h>
#include "FileManager.h"

class OSui
{
protected:
  FileManager FM;

  std::string user;
  std::string group;
  std::string cur_dir;
  TCHAR computerName[MAX_COMPUTERNAME_LENGTH + 1]; // 本机名
  short uid;
  short gid;
  bool is_login;

  void initial();
  void format();
  void mkdir(const std::string &path);
  void cd(const std::string &path);
  void ls();
  void rm(const std::string &path);
  void touch(const std::string &path);
  void cat(const std::string &path);
  void pwd();
  void chmod(const std::string &path, int mod);
  void login(const std::string &uname, const std::string &psw);
  void logout();
  void adduser(const std::string &uname, const std::string &pwd, std::string gname = "");
  void deluser(const std::string &uname);
  void addgroup(const std::string &gname);
  void delgroup(const std::string &gname);
  void df();
  void vim(const std::string &path);
  void write(const std::string &path, const std::string &content, int pos);
  void win2wlos(const std::string &fname1, const std::string &fname2);
  void wlos2win(const std::string &fname1, const std::string &fname2);

  void cls();
  void help();

public:
  OSui();
  ~OSui();
  void RunOS();
};