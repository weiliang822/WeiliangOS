#include "FileManager.h"

/*==========================class FileManager===============================*/
FileManager::FileManager()
{
  std::cout << "正在初始化文件管理模块......\n";
  this->Initialize();
  std::cout << "文件管理模块初始化完成......\n";
}

FileManager::~FileManager() {}

void FileManager::_read(Inode &inode, char *buf, int offset, int len)
{
  if (offset + len > inode.i_size)
    throw std::runtime_error("读文件越界");

  if (inode.i_size == 0)
    return;

  char *filedata = new char[inode.i_size]{0};
  int pos = 0; // 当前读到的位置

  // 直接
  for (int i = 0; i < 6 && pos < offset + len; i++)
  {
    if (pos + BLOCK_SIZE < offset + len)
    {
      this->m_FileSystem.m_BufferManager.bread((char *)filedata + pos, DATA_START_ADDR + inode.i_addr[i] * BLOCK_SIZE, BLOCK_SIZE);
      pos += BLOCK_SIZE;
    }
    else
    {
      this->m_FileSystem.m_BufferManager.bread((char *)filedata + pos, DATA_START_ADDR + inode.i_addr[i] * BLOCK_SIZE, offset + len - pos);
      pos = offset + len;
    }
  }

  // 一级索引
  for (int i = 6; i < 8 && pos < offset + len; i++)
  {
    int *primary_index = new int[BLOCK_SIZE / sizeof(int)]{0}; // 申请一级索引数组
    // 读入一级索引内容
    this->m_FileSystem.m_BufferManager.bread((char *)primary_index, DATA_START_ADDR + inode.i_addr[i] * BLOCK_SIZE, BLOCK_SIZE);
    for (int j = 0; j < BLOCK_SIZE / sizeof(int) && pos < offset + len; j++)
    {
      if (pos + BLOCK_SIZE < offset + len)
      {
        this->m_FileSystem.m_BufferManager.bread((char *)filedata + pos, DATA_START_ADDR + primary_index[j] * BLOCK_SIZE, BLOCK_SIZE);
        pos += BLOCK_SIZE;
      }
      else
      {
        this->m_FileSystem.m_BufferManager.bread((char *)filedata + pos, DATA_START_ADDR + primary_index[j] * BLOCK_SIZE, offset + len - pos);
        pos = offset + len;
      }
    }
    delete[] primary_index; // 释放一级索引数组
  }

  // 二级索引
  for (int i = 8; i < 10 && pos < offset + len; i++)
  {
    int *secondary_index = new int[BLOCK_SIZE / sizeof(int)]{0}; // 申请二级索引数组
    this->m_FileSystem.m_BufferManager.bread((char *)secondary_index, DATA_START_ADDR + inode.i_addr[i] * BLOCK_SIZE, BLOCK_SIZE);
    for (int j = 0; j < BLOCK_SIZE / sizeof(int) && pos < offset + len; j++)
    {
      int *primary_index = new int[BLOCK_SIZE / sizeof(int)]{0}; // 申请一级索引数组
      this->m_FileSystem.m_BufferManager.bread((char *)primary_index, DATA_START_ADDR + secondary_index[j] * BLOCK_SIZE, BLOCK_SIZE);
      for (int k = 0; k < BLOCK_SIZE / sizeof(int) && pos < offset + len; k++)
      {
        if (pos + BLOCK_SIZE < offset + len)
        {
          this->m_FileSystem.m_BufferManager.bread((char *)filedata + pos, DATA_START_ADDR + primary_index[k] * BLOCK_SIZE, BLOCK_SIZE);
          pos += BLOCK_SIZE;
        }
        else
        {
          this->m_FileSystem.m_BufferManager.bread((char *)filedata + pos, DATA_START_ADDR + primary_index[k] * BLOCK_SIZE, offset + len - pos);
          pos = offset + len;
        }
      }
      delete[] primary_index; // 释放一级索引数组
    }
    delete[] secondary_index; // 释放二级索引数组
  }
  memcpy_s(buf, len, filedata + offset, len);
  delete[] filedata;
  inode.i_atime = int(time(NULL)); // 最后访问时间
}

void FileManager::_write(Inode &inode, const char *buf, int offset, int len)
{
  if (offset + len > Inode::HUGE_FILE_BLOCK * BLOCK_SIZE)
  {
    std::cout << "error : 写文件过大";
    return;
  }

  int new_sz;
  if (offset + len > inode.i_size)
    new_sz = offset + len;
  else
    new_sz = inode.i_size;

  char *filedata = new char[new_sz + 1]{0};
  this->_read(inode, filedata, 0, inode.i_size);
  if (offset > inode.i_size)
    memset(filedata + inode.i_size, 0, offset - inode.i_size);
  memcpy_s(filedata + offset, len, buf, len);

  // 重写索引
  int pre_b_num = (inode.i_size + BLOCK_SIZE - 1) / BLOCK_SIZE; // 原文件盘块数
  int cur_b_num = (new_sz + BLOCK_SIZE - 1) / BLOCK_SIZE;       // 新文件盘块数
  int b_cnt = 0;

  // 直接
  for (int i = 0; i < 6 && b_cnt < cur_b_num; i++)
  {
    if (b_cnt < pre_b_num)
      b_cnt++;
    else
    {
      int b_no = this->m_FileSystem.BAlloc();
      inode.i_addr[i] = b_no;
      b_cnt++;
    }
  }

  // 一级索引
  for (int i = 6; i < 8 && b_cnt < cur_b_num; i++)
  {
    int *primary_index = new int[BLOCK_SIZE / sizeof(int)]{0}; // 申请一级索引数组
    // 读入一级索引内容
    if (b_cnt < pre_b_num)
      this->m_FileSystem.m_BufferManager.bread((char *)primary_index, DATA_START_ADDR + inode.i_addr[i] * BLOCK_SIZE, BLOCK_SIZE);
    else
    {
      int b_no = this->m_FileSystem.BAlloc();
      inode.i_addr[i] = b_no;
    }
    for (int j = 0; j < BLOCK_SIZE / sizeof(int) && b_cnt < cur_b_num; j++)
    {
      if (b_cnt < pre_b_num)
        b_cnt++;
      else
      {
        int b_no = this->m_FileSystem.BAlloc();
        primary_index[j] = b_no;
        b_cnt++;
      }
    }
    this->m_FileSystem.m_BufferManager.bwrite((const char *)primary_index, DATA_START_ADDR + inode.i_addr[i] * BLOCK_SIZE, BLOCK_SIZE);
    delete[] primary_index; // 释放一级索引数组
  }

  // 二级索引
  for (int i = 8; i < 10 && b_cnt < cur_b_num; i++)
  {
    int *secondary_index = new int[BLOCK_SIZE / sizeof(int)]{0}; // 申请二级索引数组
    if (b_cnt < pre_b_num)
      this->m_FileSystem.m_BufferManager.bread((char *)secondary_index, DATA_START_ADDR + inode.i_addr[i] * BLOCK_SIZE, BLOCK_SIZE);
    else
    {
      int b_no = this->m_FileSystem.BAlloc();
      inode.i_addr[i] = b_no;
    }
    for (int j = 0; j < BLOCK_SIZE / sizeof(int) && b_cnt < cur_b_num; j++)
    {
      int *primary_index = new int[BLOCK_SIZE / sizeof(int)]{0}; // 申请一级索引数组
      if (b_cnt < pre_b_num)
        this->m_FileSystem.m_BufferManager.bread((char *)primary_index, DATA_START_ADDR + secondary_index[j] * BLOCK_SIZE, BLOCK_SIZE);
      else
      {
        int b_no = this->m_FileSystem.BAlloc();
        secondary_index[j] = b_no;
      }
      for (int k = 0; k < BLOCK_SIZE / sizeof(int) && b_cnt < cur_b_num; k++)
      {
        if (b_cnt < pre_b_num)
          b_cnt++;
        else
        {
          int b_no = this->m_FileSystem.BAlloc();
          primary_index[k] = b_no;
          b_cnt++;
        }
      }
      this->m_FileSystem.m_BufferManager.bwrite((const char *)primary_index, DATA_START_ADDR + secondary_index[j] * BLOCK_SIZE, BLOCK_SIZE);
      delete[] primary_index; // 释放一级索引数组
    }
    this->m_FileSystem.m_BufferManager.bwrite((const char *)secondary_index, DATA_START_ADDR + inode.i_addr[i] * BLOCK_SIZE, BLOCK_SIZE);
    delete[] secondary_index; // 释放二级索引数组
  }

  inode.i_size = new_sz;

  int pos = 0; // 当前读到的位置

  // 直接
  for (int i = 0; i < 6 && pos < inode.i_size; i++)
  {
    if (pos + BLOCK_SIZE < inode.i_size)
    {
      this->m_FileSystem.m_BufferManager.bwrite((const char *)filedata + pos, DATA_START_ADDR + inode.i_addr[i] * BLOCK_SIZE, BLOCK_SIZE);
      pos += BLOCK_SIZE;
    }
    else
    {
      this->m_FileSystem.m_BufferManager.bwrite((const char *)filedata + pos, DATA_START_ADDR + inode.i_addr[i] * BLOCK_SIZE, inode.i_size - pos);
      pos = inode.i_size;
    }
  }

  // 一级索引
  for (int i = 6; i < 8 && pos < inode.i_size; i++)
  {
    int *primary_index = new int[BLOCK_SIZE / sizeof(int)]{0}; // 申请一级索引数组
    // 读入一级索引内容
    this->m_FileSystem.m_BufferManager.bread((char *)primary_index, DATA_START_ADDR + inode.i_addr[i] * BLOCK_SIZE, BLOCK_SIZE);
    for (int j = 0; j < BLOCK_SIZE / sizeof(int) && pos < inode.i_size; j++)
    {
      if (pos + BLOCK_SIZE < inode.i_size)
      {
        this->m_FileSystem.m_BufferManager.bwrite((char *)filedata + pos, DATA_START_ADDR + primary_index[j] * BLOCK_SIZE, BLOCK_SIZE);
        pos += BLOCK_SIZE;
      }
      else
      {
        this->m_FileSystem.m_BufferManager.bwrite((char *)filedata + pos, DATA_START_ADDR + primary_index[j] * BLOCK_SIZE, inode.i_size - pos);
        pos = inode.i_size;
      }
    }
    delete[] primary_index; // 释放一级索引数组
  }

  // 二级索引
  for (int i = 8; i < 10 && pos < inode.i_size; i++)
  {
    int *secondary_index = new int[BLOCK_SIZE / sizeof(int)]{0}; // 申请二级索引数组
    this->m_FileSystem.m_BufferManager.bread((char *)secondary_index, DATA_START_ADDR + inode.i_addr[i] * BLOCK_SIZE, BLOCK_SIZE);
    for (int j = 0; j < BLOCK_SIZE / sizeof(int) && pos < inode.i_size; j++)
    {
      int *primary_index = new int[BLOCK_SIZE / sizeof(int)]{0}; // 申请一级索引数组
      this->m_FileSystem.m_BufferManager.bread((char *)primary_index, DATA_START_ADDR + secondary_index[j] * BLOCK_SIZE, BLOCK_SIZE);
      for (int k = 0; k < BLOCK_SIZE / sizeof(int) && pos < inode.i_size; k++)
      {
        if (pos + BLOCK_SIZE < inode.i_size)
        {
          this->m_FileSystem.m_BufferManager.bwrite((const char *)filedata + pos, DATA_START_ADDR + primary_index[k] * BLOCK_SIZE, BLOCK_SIZE);
          pos += BLOCK_SIZE;
        }
        else
        {
          this->m_FileSystem.m_BufferManager.bwrite((const char *)filedata + pos, DATA_START_ADDR + primary_index[k] * BLOCK_SIZE, inode.i_size - pos);
          pos = inode.i_size;
        }
      }
      delete[] primary_index; // 释放一级索引数组
    }
    delete[] secondary_index; // 释放二级索引数组
  }

  delete[] filedata;

  inode.i_atime = int(time(NULL)); // 最后访问时间
  inode.i_mtime = int(time(NULL)); // 最后修改时间
}

void FileManager::_fdelete(Inode &inode)
{
  int pos = 0;

  // 直接
  for (int i = 0; i < 6 && pos < inode.i_size; i++)
  {
    this->m_FileSystem.BFree(inode.i_addr[i]);
    pos += BLOCK_SIZE;
  }

  // 一级索引
  for (int i = 6; i < 8 && pos < inode.i_size; i++)
  {
    int *primary_index = new int[BLOCK_SIZE / sizeof(int)]{0}; // 申请一级索引数组
    // 读入一级索引内容
    this->m_FileSystem.m_BufferManager.bread((char *)primary_index, DATA_START_ADDR + inode.i_addr[i] * BLOCK_SIZE, BLOCK_SIZE);
    for (int j = 0; j < BLOCK_SIZE / sizeof(int) && pos < inode.i_size; j++)
    {
      this->m_FileSystem.BFree(primary_index[j]);
      pos += BLOCK_SIZE;
    }
    this->m_FileSystem.BFree(inode.i_addr[i]);
    delete[] primary_index; // 释放一级索引数组
  }

  // 二级索引
  for (int i = 8; i < 10 && pos < inode.i_size; i++)
  {
    int *secondary_index = new int[BLOCK_SIZE / sizeof(int)]{0}; // 申请二级索引数组
    this->m_FileSystem.m_BufferManager.bread((char *)secondary_index, DATA_START_ADDR + inode.i_addr[i] * BLOCK_SIZE, BLOCK_SIZE);
    for (int j = 0; j < BLOCK_SIZE / sizeof(int) && pos < inode.i_size; j++)
    {
      int *primary_index = new int[BLOCK_SIZE / sizeof(int)]{0}; // 申请一级索引数组
      this->m_FileSystem.m_BufferManager.bread((char *)primary_index, DATA_START_ADDR + secondary_index[j] * BLOCK_SIZE, BLOCK_SIZE);
      for (int k = 0; k < BLOCK_SIZE / sizeof(int) && pos < inode.i_size; k++)
      {
        this->m_FileSystem.BFree(primary_index[k]);
        pos += BLOCK_SIZE;
      }
      this->m_FileSystem.BFree(secondary_index[j]);
      delete[] primary_index; // 释放一级索引数组
    }
    this->m_FileSystem.BFree(inode.i_addr[i]);
    delete[] secondary_index; // 释放二级索引数组
  }
  inode.i_size = 0;
  // inode.i_mode &= (~(1 << 12));
  this->m_FileSystem.IFree(inode);
  // inode.i_atime = int(time(NULL)); // 最后访问时间
  // inode.i_mtime = int(time(NULL)); // 最后修改时间
}

void FileManager::_ddelete(Inode &inode)
{
  // 若不是目录
  if ((inode.i_mode & Inode::IFDIR) == 0)
    this->_fdelete(inode);
  else
  {
    DirectoryEntry *dir_li = new DirectoryEntry[inode.i_size / DIRECTORY_ENTRY_SIZE];
    this->_read(inode, (char *)dir_li, 0, inode.i_size);
    for (int i = 0; i < inode.i_size / DIRECTORY_ENTRY_SIZE; i++)
    {
      if (strcmp(dir_li[i].m_name, ".") == 0 || strcmp(dir_li[i].m_name, "..") == 0)
        continue;
      else
      {
        Inode sub_dir = this->m_FileSystem.ILoad(dir_li[i].m_ino);
        _ddelete(sub_dir);
        this->m_FileSystem.IFree(sub_dir);
      }
    }
    _fdelete(inode);
    delete[] dir_li;
  }
}

Inode FileManager::_find(Inode &inode, const std::string &fname)
{
  if (!(inode.i_mode & Inode::IFDIR))
  {
    // std::cerr << "找不到该文件" << '\n';
    // return;
    throw std::runtime_error("非目录文件");
  }

  DirectoryEntry *dir_li = new DirectoryEntry[inode.i_size / DIRECTORY_ENTRY_SIZE];
  this->_read(inode, (char *)dir_li, 0, inode.i_size);
  for (int i = 0; i < inode.i_size / DIRECTORY_ENTRY_SIZE; i++)
  {
    if (fname == dir_li[i].m_name)
    {
      int res = dir_li[i].m_ino;
      delete[] dir_li;
      return this->m_FileSystem.ILoad(res);
    }
  }
  delete[] dir_li;
  throw std::runtime_error("找不到该文件");
  // std::cerr << "找不到该文件" << '\n';
}

File *FileManager::fopen(const std::string &path, short uid, short gid)
{
  Inode cur_inode = this->m_FileSystem.ILoad(int(ROOT_DIR_INO));
  std::vector<std::string> paths = splitstring(path, "/");
  for (int i = 0; i < paths.size(); i++)
    cur_inode = _find(cur_inode, paths[i]);

  if (cur_inode.i_mode & Inode::IFDIR)
    throw std::runtime_error("不能打开目录");

  File *fp = new File;
  Inode *ip = new Inode;
  memcpy_s(ip, sizeof(Inode), &cur_inode, sizeof(Inode));
  fp->f_uid = uid;
  fp->f_gid = gid;
  fp->f_inode = ip;
  fp->f_offset = 0;
  return fp;
}

void FileManager::fclose(File *fp)
{
  this->m_FileSystem.ISave(*fp->f_inode);
  delete[] fp->f_inode;
  delete fp;
}

void FileManager::fread(char *buf, int len, File *fp)
{
  int r_shift;                         // 右移量
  if (fp->f_uid == fp->f_inode->i_uid) // 文件主
    r_shift = 6;
  else if (fp->f_gid == fp->f_inode->i_gid) // 文件主同组用户
    r_shift = 3;
  else // 其他用户
    r_shift = 0;

  if (((fp->f_inode->i_mode >> r_shift) & 4) == 0 && fp->f_uid != 0)
  {
    std::cout << "权限不足：缺少读权限\n";
    return;
  } // 无读权限且不是root用户

  if (fp->f_offset + len > fp->f_inode->i_size)
  {
    std::cerr << "waring : 读文件范围超出文件大小，已自动修正至文件末尾\n";
    len = fp->f_inode->i_size - fp->f_offset;
  }
  _read(*fp->f_inode, buf, fp->f_offset, len);
  fp->f_offset += len;
}

void FileManager::fwrite(const char *buf, int len, File *fp)
{
  int r_shift;                         // 右移量
  if (fp->f_uid == fp->f_inode->i_uid) // 文件主
    r_shift = 6;
  else if (fp->f_gid == fp->f_inode->i_gid) // 文件主同组用户
    r_shift = 3;
  else // 其他用户
    r_shift = 0;

  if (((fp->f_inode->i_mode >> r_shift) & 2) == 0 && fp->f_uid != 0)
  {
    std::cout << "权限不足：缺少写权限\n";
    return;
  } // 无读权限且不是root用户

  this->_write(*fp->f_inode, buf, fp->f_offset, len);
  fp->f_offset += len;
}

void FileManager::freplace(const char *buf, int len, File *fp)
{
  int r_shift;                         // 右移量
  if (fp->f_uid == fp->f_inode->i_uid) // 文件主
    r_shift = 6;
  else if (fp->f_gid == fp->f_inode->i_gid) // 文件主同组用户
    r_shift = 3;
  else // 其他用户
    r_shift = 0;

  if (((fp->f_inode->i_mode >> r_shift) & 2) == 0 && fp->f_uid != 0)
  {
    std::cout << "权限不足：缺少写权限\n";
    return;
  } // 无读权限且不是root用户

  char *old_data = new char[fp->f_inode->i_size + 1];
  this->_read(*fp->f_inode, old_data, 0, fp->f_inode->i_size);
  _fdelete(*fp->f_inode);
  char *new_data = new char[fp->f_offset + len + 1]{0};
  memcpy_s(new_data, fp->f_offset, old_data, fp->f_offset);
  memcpy_s(new_data + fp->f_offset, len, buf, len);
  this->_write(*fp->f_inode, new_data, 0, fp->f_offset + len);
  delete[] old_data;
  delete[] new_data;
  fp->f_offset += len;
}

void FileManager::fcreate(const std::string &path, short uid, short gid)
{
  Inode cur_inode = this->m_FileSystem.ILoad(int(ROOT_DIR_INO));
  std::vector<std::string> paths = splitstring(path, "/");
  std::string fname = paths[paths.size() - 1];

  if (fname.size() > DirectoryEntry::DIRSIZ)
  {
    std::cout << "error : 新增文件名超过最大长度\n";
    return;
  }

  for (int i = 0; i < paths.size() - 1; i++)
    cur_inode = _find(cur_inode, paths[i]);

  if (!(cur_inode.i_mode & Inode::IFDIR))
    throw std::runtime_error("当前路径非目录");

  int r_shift;                // 右移量
  if (uid == cur_inode.i_uid) // 文件主
    r_shift = 6;
  else if (gid == cur_inode.i_gid) // 文件主同组用户
    r_shift = 3;
  else // 其他用户
    r_shift = 0;

  if (((cur_inode.i_mode >> r_shift) & 2) == 0 && uid != 0)
  {
    std::cout << "权限不足：缺少写权限\n";
    return;
  } // 无写权限且不是root用户

  if (cur_inode.i_size >= Inode::HUGE_FILE_BLOCK * BLOCK_SIZE)
  {
    std::cout << "error : 当前目录已写满\n";
    return;
  }

  DirectoryEntry *dir_li = new DirectoryEntry[cur_inode.i_size / DIRECTORY_ENTRY_SIZE];
  this->_read(cur_inode, (char *)dir_li, 0, cur_inode.i_size);

  // 若当前目录已存在该文件，则进行替换
  for (int i = 0; i < cur_inode.i_size / DIRECTORY_ENTRY_SIZE; i++)
    if (dir_li[i].m_name == fname)
      this->fdelete(path, uid, gid);

  delete[] dir_li;

  Inode new_inode = this->m_FileSystem.IAlloc();
  new_inode.i_mode = Inode::IALLOC | Inode::FILE_DEF_PER;
  new_inode.i_nlink = 1;
  new_inode.i_uid = uid;
  new_inode.i_gid = gid;
  new_inode.i_size = 0;
  new_inode.i_atime = int(time(NULL)); // 最后访问时间
  new_inode.i_mtime = int(time(NULL)); // 最后修改时间
  this->m_FileSystem.ISave(new_inode);

  DirectoryEntry new_dir_item;
  strcpy_s(new_dir_item.m_name, fname.c_str());
  new_dir_item.m_ino = new_inode.i_no;
  // 写入新的目录项
  this->_write(cur_inode, (const char *)&new_dir_item, cur_inode.i_size, DIRECTORY_ENTRY_SIZE);
  this->m_FileSystem.ISave(cur_inode);
}

void FileManager::fdelete(const std::string &path, short uid, short gid)
{
  Inode cur_inode = this->m_FileSystem.ILoad(ROOT_DIR_INO);
  std::vector<std::string> paths = splitstring(path, "/");
  std::string fname = paths[paths.size() - 1];

  for (int i = 0; i < paths.size() - 1; i++)
    cur_inode = _find(cur_inode, paths[i]);

  if (!(cur_inode.i_mode & Inode::IFDIR))
    throw std::runtime_error("当前路径非目录");

  int r_shift;                // 右移量
  if (uid == cur_inode.i_uid) // 文件主
    r_shift = 6;
  else if (gid == cur_inode.i_gid) // 文件主同组用户
    r_shift = 3;
  else // 其他用户
    r_shift = 0;

  if (((cur_inode.i_mode >> r_shift) & 2) == 0 && uid != 0)
  {
    std::cout << "权限不足：缺少写权限\n";
    return;
  } // 无写权限且不是root用户

  DirectoryEntry *dir_li = new DirectoryEntry[cur_inode.i_size / DIRECTORY_ENTRY_SIZE];
  this->_read(cur_inode, (char *)dir_li, 0, cur_inode.i_size);

  int f_ino = -1; // 要删除的文件的inode编号
  int pos;        // 要删除文件在目录下的位置
  for (int i = 0; i < cur_inode.i_size / DIRECTORY_ENTRY_SIZE; i++)
    if (dir_li[i].m_name == fname)
    {
      f_ino = dir_li[i].m_ino;
      pos = i;
      break;
    }

  if (f_ino == -1)
  {
    std::cout << "error : 找不到要删除的文件\n";
    return;
  }

  Inode dinode = this->m_FileSystem.ILoad(f_ino); // 加载要删除的inode
  if (dinode.i_mode & Inode::IFDIR)
  {
    delete[] dir_li;
    std::cout << "error : 目录文件不能用fdelete删除\n";
    return;
  }

  _fdelete(dinode);
  this->m_FileSystem.IFree(dinode);

  // 目录下文件前移
  for (int i = pos; i < cur_inode.i_size / DIRECTORY_ENTRY_SIZE - 1; i++)
    dir_li[i] = dir_li[i + 1];

  int old_sz = cur_inode.i_size;
  this->_fdelete(cur_inode);
  this->_write(cur_inode, (const char *)dir_li, 0, old_sz - DIRECTORY_ENTRY_SIZE); // 重写当前目录inode
  delete[] dir_li;
  this->m_FileSystem.ISave(cur_inode);
}

void FileManager::flseek(File *fp, int offset, int origin)
{
  if (origin == SEEK_SET)
    fp->f_offset = offset; // 重置
  else if (origin == SEEK_CUR)
    fp->f_offset = fp->f_offset + offset; // 从当前
  else
    fp->f_offset = fp->f_inode->i_size - 1 + offset; // 从最后
}

int FileManager::ftell(File *fp)
{
  return fp->f_offset;
}

void FileManager::dcreate(const std::string &path, short uid, short gid)
{
  Inode cur_inode = this->m_FileSystem.ILoad(int(ROOT_DIR_INO));
  std::vector<std::string> paths = splitstring(path, "/");
  std::string dname = paths[paths.size() - 1]; // 新建文件夹名

  if (dname.size() > DirectoryEntry::DIRSIZ)
  {
    std::cout << "error : 新增目录名超过最大长度\n";
    return;
  }

  for (int i = 0; i < paths.size() - 1; i++)
    cur_inode = _find(cur_inode, paths[i]);

  if (!(cur_inode.i_mode & Inode::IFDIR))
    throw std::runtime_error("当前路径非目录");

  int r_shift;                // 右移量
  if (uid == cur_inode.i_uid) // 文件主
    r_shift = 6;
  else if (gid == cur_inode.i_gid) // 文件主同组用户
    r_shift = 3;
  else // 其他用户
    r_shift = 0;

  if (((cur_inode.i_mode >> r_shift) & 2) == 0 && uid != 0)
  {
    std::cout << "权限不足：缺少写权限\n";
    return;
  } // 无写权限且不是root用户

  if (cur_inode.i_size >= Inode::HUGE_FILE_BLOCK * BLOCK_SIZE)
  {
    std::cout << "error : 当前目录已写满\n";
    return;
  }

  DirectoryEntry *dir_li = new DirectoryEntry[cur_inode.i_size / DIRECTORY_ENTRY_SIZE];
  this->_read(cur_inode, (char *)dir_li, 0, cur_inode.i_size);

  // 若当前目录已存在该目录
  for (int i = 0; i < cur_inode.i_size / DIRECTORY_ENTRY_SIZE; i++)
    if (dir_li[i].m_name == dname)
    {
      delete[] dir_li;
      std::cout << "error : 当前目录下已有该目录\n";
      return;
    }

  delete[] dir_li;

  // 创建新inode结点
  Inode new_dir_inode = this->m_FileSystem.IAlloc();
  new_dir_inode.i_mode = Inode::IALLOC | Inode::IFDIR | Inode::DIR_DEF_PER;
  new_dir_inode.i_nlink = 1;
  new_dir_inode.i_uid = uid;
  new_dir_inode.i_gid = gid;
  new_dir_inode.i_size = 0;
  new_dir_inode.i_atime = int(time(NULL)); // 最后访问时间
  new_dir_inode.i_mtime = int(time(NULL)); // 最后修改时间

  // 在目录下添加本目录和父目录
  DirectoryEntry *new_dir_li = new DirectoryEntry[2];
  strcpy_s(new_dir_li[0].m_name, ".");
  new_dir_li[0].m_ino = new_dir_inode.i_no;
  strcpy_s(new_dir_li[1].m_name, "..");
  new_dir_li[1].m_ino = cur_inode.i_no;
  this->_write(new_dir_inode, (const char *)new_dir_li, 0, 2 * DIRECTORY_ENTRY_SIZE);
  delete[] new_dir_li;
  this->m_FileSystem.ISave(new_dir_inode);

  // 更新父目录
  DirectoryEntry new_dir_item;
  strcpy_s(new_dir_item.m_name, dname.c_str());
  new_dir_item.m_ino = new_dir_inode.i_no;
  cur_inode.i_nlink++;
  this->_write(cur_inode, (const char *)&new_dir_item, cur_inode.i_size, DIRECTORY_ENTRY_SIZE);
  this->m_FileSystem.ISave(cur_inode);
}

void FileManager::ddelete(const std::string &path, short uid, short gid)
{
  Inode cur_inode = this->m_FileSystem.ILoad(int(ROOT_DIR_INO));
  std::vector<std::string> paths = splitstring(path, "/");
  std::string dname = paths[paths.size() - 1]; // 删除文件夹名

  if (dname == "." || dname == "..")
  {
    std::cout << "error : 不能对本目录或上级目录执行删除操作\n";
    return;
  }

  for (int i = 0; i < paths.size() - 1; i++)
    cur_inode = _find(cur_inode, paths[i]);

  if (!(cur_inode.i_mode & Inode::IFDIR))
    throw std::runtime_error("当前路径非目录");

  int r_shift;                // 右移量
  if (uid == cur_inode.i_uid) // 文件主
    r_shift = 6;
  else if (gid == cur_inode.i_gid) // 文件主同组用户
    r_shift = 3;
  else // 其他用户
    r_shift = 0;

  if (((cur_inode.i_mode >> r_shift) & 2) == 0 && uid != 0)
  {
    std::cout << "权限不足：缺少写权限\n";
    return;
  } // 无写权限且不是root用户

  DirectoryEntry *dir_li = new DirectoryEntry[cur_inode.i_size / DIRECTORY_ENTRY_SIZE];
  this->_read(cur_inode, (char *)dir_li, 0, cur_inode.i_size);

  int d_ino = -1; // 要删除的目录的inode编号
  int pos;        // 要删除文件在目录下的位置
  for (int i = 0; i < cur_inode.i_size / DIRECTORY_ENTRY_SIZE; i++)
    if (dir_li[i].m_name == dname)
    {
      d_ino = dir_li[i].m_ino;
      pos = i;
      break;
    }

  if (d_ino == -1)
  {
    std::cout << "error : 找不到要删除的目录\n";
    return;
  }

  Inode dinode = this->m_FileSystem.ILoad(d_ino); // 加载要删除的inode
  if (!(dinode.i_mode & Inode::IFDIR))
  {
    // delete[] dir_li;
    // throw std::runtime_error("非目录文件不能用ddelete删除");
    this->fdelete(path, uid, gid);
    return;
  }

  _ddelete(dinode);
  this->m_FileSystem.IFree(dinode);

  // 目录下文件前移
  for (int i = pos; i < cur_inode.i_size / DIRECTORY_ENTRY_SIZE - 1; i++)
    dir_li[i] = dir_li[i + 1];

  int old_sz = cur_inode.i_size;
  this->_fdelete(cur_inode);
  cur_inode.i_nlink--;
  this->_write(cur_inode, (const char *)dir_li, 0, old_sz - DIRECTORY_ENTRY_SIZE); // 重写当前目录inode
  delete[] dir_li;
  this->m_FileSystem.ISave(cur_inode);
}

void FileManager::ChMod(const std::string &path, int mode, short uid, short gid)
{
  Inode cur_inode = this->m_FileSystem.ILoad(int(ROOT_DIR_INO));
  std::vector<std::string> paths = splitstring(path, "/");

  for (int i = 0; i < paths.size(); i++)
    cur_inode = _find(cur_inode, paths[i]);

  if (uid != cur_inode.i_uid && uid != 0) // 只有文件主和root用户能修改权限
  {
    std::cout << "error : 权限不足\n";
    return;
  }

  cur_inode.i_mode = (cur_inode.i_mode >> 9 << 9) | mode;
  this->m_FileSystem.ISave(cur_inode);
}

void FileManager::LS(const std::string &path, short uid, short gid)
{
  Inode cur_inode = this->m_FileSystem.ILoad(int(ROOT_DIR_INO));
  std::vector<std::string> paths = splitstring(path, "/");

  for (int i = 0; i < paths.size(); i++)
    cur_inode = _find(cur_inode, paths[i]);

  if (!(cur_inode.i_mode & Inode::IFDIR))
    throw std::runtime_error("当前路径非目录");

  int r_shift;                // 右移量
  if (uid == cur_inode.i_uid) // 文件主
    r_shift = 6;
  else if (gid == cur_inode.i_gid) // 文件主同组用户
    r_shift = 3;
  else // 其他用户
    r_shift = 0;

  if (((cur_inode.i_mode >> r_shift) & 4) == 0 && uid != 0) // 无写权限且不是root用户
  {
    std::cout << "权限不足：缺少读权限\n";
    return;
  }

  DirectoryEntry *dir_li = new DirectoryEntry[cur_inode.i_size / DIRECTORY_ENTRY_SIZE];
  this->_read(cur_inode, (char *)dir_li, 0, cur_inode.i_size);

  std::vector<std::string> res;

  for (int i = 0; i < cur_inode.i_size / DIRECTORY_ENTRY_SIZE; i++)
  {
    Inode sub_inode = this->m_FileSystem.ILoad(dir_li[i].m_ino);

    std::string permission;
    if (sub_inode.i_mode & Inode::IFDIR)
      permission += 'd'; // 文件夹
    else
      permission += '-'; // 文件

    // 文件读写权限
    for (int j = 8; j >= 0; j--)
    {
      if ((sub_inode.i_mode >> j) & 1)
      {
        if (j % 3 == 2)
          permission += 'r';
        else if (j % 3 == 1)
          permission += 'w';
        else
          permission += 'x';
      }
      else
        permission += '-';
    }
    std::cout << permission << " ";

    // 链接数
    // permission += itoa(sub_inode.i_nlink);
    std::cout << sub_inode.i_nlink << " ";

    // 获取用户名
    std::string uname;
    File *userf = this->fopen("/etc/users.txt", 0, 0);
    char *data = new char[userf->f_inode->i_size + 1]{0};
    this->fread(data, userf->f_inode->i_size, userf);
    std::string sdata = data;
    delete[] data;
    this->fclose(userf);
    std::vector<std::string> users;
    users = splitstring(sdata, "\n");

    for (int i = 0; i < users.size(); i++)
    {
      std::vector<std::string> umsg = splitstring(users[i], "-");
      if (stoi(umsg[2]) == sub_inode.i_uid)
      {
        uname = umsg[0];
        break;
      }
    }

    // 获取组名
    std::string gname;
    File *groupf = this->fopen("/etc/groups.txt", 0, 0);
    data = new char[groupf->f_inode->i_size + 1]{0};
    this->fread(data, groupf->f_inode->i_size, groupf);
    sdata = data;
    delete[] data;
    this->fclose(groupf);
    std::vector<std::string> groups;
    groups = splitstring(sdata, "\n");

    for (int i = 0; i < groups.size(); i++)
    {
      std::vector<std::string> gmsg = splitstring(groups[i], "-");
      if (stoi(gmsg[1]) == sub_inode.i_gid)
      {
        gname = gmsg[0];
        break;
      }
    }

    std::cout << uname << "  " << gname << " " << sub_inode.i_size << " ";
    std::cout << dir_li[i].m_name << "  ";

    tm stdT; // 存储时间
    time_t time = sub_inode.i_mtime;
    localtime_s(&stdT, (const time_t *)&time);

    std::cout << 1900 + stdT.tm_year << "年" << stdT.tm_mon + 1 << "月" << stdT.tm_mday << "日" << stdT.tm_hour << "时" << stdT.tm_min << "分" << stdT.tm_sec << "秒";
    std::cout << '\n';
    // res.push_back(permission);
  }
  return;
}

bool FileManager::Enter(const std::string &path, short uid, short gid)
{
  Inode cur_inode = this->m_FileSystem.ILoad(int(ROOT_DIR_INO));
  std::vector<std::string> paths = splitstring(path, "/");

  for (int i = 0; i <= paths.size(); i++)
  {
    int r_shift;                // 右移量
    if (uid == cur_inode.i_uid) // 文件主
      r_shift = 6;
    else if (gid == cur_inode.i_gid) // 文件主同组用户
      r_shift = 3;
    else // 其他用户
      r_shift = 0;

    if (((cur_inode.i_mode >> r_shift) & 1) == 0 && uid != 0)
    {
      std::cout << "权限不足：缺少执行权限\n";
      return false;
    } // 无写权限且不是root用户

    if (!(cur_inode.i_mode & Inode::IFDIR))
    {
      std::cout << "error : 目标路径非目录文件\n";
      return false;
    }

    if (i < paths.size())
      cur_inode = _find(cur_inode, paths[i]);
  }

  return true;
}

void FileManager::Initialize()
{
  std::ifstream fin;
  fin.open(WLOS_DISK_NAME, std::ios::in | std::ios::binary);
  if (!fin.is_open())
    this->fformat();
  else
    this->m_FileSystem.m_BufferManager.bread((char *)&this->m_FileSystem.spb, SUPERBLOCK_START_ADDR, SUPERBLOCK_SIZE);
}

void FileManager::fformat()
{
  // 创建新的磁盘卷
  std::ofstream fout;
  fout.open(WLOS_DISK_NAME, std::ios::out | std::ios::binary);
  if (!fout.is_open())
    throw std::runtime_error("磁盘映像输出文件创建失败\n");
  fout.seekp(WLOS_DISK_SIZE - 1, std::ios::beg);
  fout.write("", sizeof(char));
  fout.close();

  this->m_FileSystem.fformat();

  this->dcreate("/bin", 0, 0);
  this->dcreate("/etc", 0, 0);
  this->dcreate("/home", 0, 0);
  this->dcreate("/dev", 0, 0);

  this->fcreate("/etc/users.txt", 0, 0);
  File *user = this->fopen("/etc/users.txt", 0, 0);
  std::string data = "root-root-0-0\n";
  this->fwrite(data.c_str(), data.length(), user);
  fclose(user);
  this->ChMod("/etc/users.txt", 0660, 0, 0);

  this->fcreate("/etc/groups.txt", 0, 0);
  File *groups = this->fopen("/etc/groups.txt", 0, 0);
  data = "root-0-0\n";
  this->fwrite(data.c_str(), data.length(), groups);
  fclose(groups);
  this->ChMod("/etc/groups.txt", 0660, 0, 0);
  return;
}

/*==========================class DirectoryEntry===============================*/
DirectoryEntry::DirectoryEntry()
{
  this->m_ino = 0;
  this->m_name[0] = '\0';
}

DirectoryEntry::~DirectoryEntry() {}
