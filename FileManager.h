#ifndef FILE_MANAGER_H
#define FILE_MANAGER_H

#include "File.h"
#include "FileSystem.h"
#include "ToolFun.h"
#include <fstream>
#include <iostream>

/*
 * 文件管理类(FileManager)
 * 封装了文件系统的各种系统调用在核心态下处理过程，
 * 如对文件的Open()、Close()、Read()、Write()等等
 * 封装了对文件系统访问的具体细节。
 */
class FileManager
{
protected:
  void _read(Inode &inode, char *buf, int offset, int len);
  void _write(Inode &inode, const char *buf, int offset, int len);
  void _fdelete(Inode &inode); // 文件删除
  void _ddelete(Inode &inode); // 目录删除
  Inode _find(Inode &inode, const std::string &fname);

public:
  FileManager();
  ~FileManager();

  File *fopen(const std::string &path, short uid, short gid);  // 打开文件
  void fclose(File *fp);                                       // 关闭文件
  void fread(char *buf, int len, File *fp);                    // 读文件
  void fwrite(const char *buf, int len, File *fp);             // 写文件
  void freplace(const char *buf, int len, File *fp);           // 替换文件
  void fcreate(const std::string &path, short uid, short gid); // 创建文件
  void fdelete(const std::string &path, short uid, short gid); // 删除文件
  void flseek(File *fp, int offset, int whence);               // 定位文件读写指针
  int ftell(File *fp);                                         // 返回当前文件指针
  void dcreate(const std::string &path, short uid, short gid); // 创建文件夹
  void ddelete(const std::string &path, short uid, short gid); // 删除文件夹

  void ChMod(const std::string &path, int mode, short uid, short gid);        // 修改文件权限
  std::vector<std::string> LS(const std::string &path, short uid, short gid); // 列出目录下文件
  bool Enter(const std::string &path, short uid, short gid);                  // 进入目录

  /*初始化*/
  void Initialize();
  void fformat();

public:
  FileSystem m_FileSystem;
};

#endif
