#ifndef FILE_H
#define FILE_H

#include "INode.h"

/*
 * 打开文件控制块File类。
 * 该结构记录了进程打开文件
 * 的读、写请求类型，文件读写位置等等。
 */
class File
{
public:
  enum FileFlags
  {
    FREAD = 0x1,  /* 读请求类型 */
    FWRITE = 0x2, /* 写请求类型 */
    FPIPE = 0x4   /* 管道类型 */
  };

public:
  unsigned int f_flag; /* 对打开文件的读、写操作要求 */
  int f_count;         /* 当前引用该文件控制块的进程数量 */
  Inode *f_inode;      /* 指向打开文件的内存Inode指针 */
  int f_offset;        /* 文件读写位置指针 */
  short f_uid;
  short f_gid;
};

#endif
