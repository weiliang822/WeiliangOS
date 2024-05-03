#include "FileSystem.h"
#include <iostream>

/*==============================class SuperBlock===================================*/

SuperBlock::SuperBlock() {}

SuperBlock::~SuperBlock() {}

/*==============================class FileSystem===================================*/
FileSystem::FileSystem()
{
  std::cout << "正在初始化文件系统模块......\n";
  Initialize();
  std::cout << "文件系统模块初始化完成......\n";
}

FileSystem::~FileSystem()
{
  this->m_BufferManager.bwrite((const char *)&this->spb, SUPERBLOCK_START_ADDR, SUPERBLOCK_SIZE);
}

void FileSystem::Initialize()
{
}

void FileSystem::fformat()
{
  // 格式化spb
  this->spb.s_isize = INODE_ZONE_SIZE / BLOCK_SIZE;       // 外存Inode区占用的盘块数
  this->spb.s_bsize = WLOS_DISK_SIZE / BLOCK_SIZE - 1024; // 盘块总数
  this->spb.s_finode = INODE_NUM;                         // 空闲inode数量
  this->spb.s_fblock = this->spb.s_bsize;                 // 空闲块数量

  this->spb.s_ninode = 0;
  // 初始化直接管理的inode，按栈方式分配
  for (int i = 99; i >= 0; i--)
    this->spb.s_inode[this->spb.s_ninode++] = i;

  this->spb.s_nfree = 1;
  memset(spb.s_free, 0, sizeof(spb.s_free));
  spb.s_free[0] = 0;

  for (int i = this->spb.s_fblock - 1; i >= 0; i--)
  {
    int cur_b_addr = DATA_START_ADDR + i * BLOCK_SIZE;
    if (spb.s_nfree == 100)
    {
      this->m_BufferManager.bwrite((const char *)&spb.s_nfree, cur_b_addr, sizeof(spb.s_nfree));
      this->m_BufferManager.bwrite((const char *)spb.s_free, cur_b_addr + int(sizeof(spb.s_nfree)), sizeof(spb.s_free));
      spb.s_nfree = 0;
      memset(spb.s_free, 0, sizeof(spb.s_free));
    }
    spb.s_free[spb.s_nfree++] = i;
  }

  this->m_BufferManager.bwrite((const char *)&spb, SUPERBLOCK_START_ADDR, sizeof(spb)); // 写入spb到磁盘

  // 根目录
  Inode root = this->IAlloc();
  int b_no = this->BAlloc();
  DirectoryEntry *dir_li = new DirectoryEntry[BLOCK_SIZE / DIRECTORY_ENTRY_SIZE];
  strcpy_s(dir_li[0].m_name, ".");
  dir_li[0].m_ino = root.i_no;
  // 目录盘块内存DirectoryEntry
  this->m_BufferManager.bwrite((const char *)dir_li, DATA_START_ADDR + b_no * BLOCK_SIZE, BLOCK_SIZE);
  delete[] dir_li;

  root.i_mode = Inode::IALLOC | Inode::IFDIR | Inode::DIR_DEF_PER;
  root.i_nlink = 1;
  root.i_uid = 0;
  root.i_gid = 0;
  root.i_size = DIRECTORY_ENTRY_SIZE;
  root.i_addr[0] = b_no;
  root.i_atime = int(time(NULL));
  root.i_mtime = int(time(NULL));
  this->ISave(root);
}

Inode FileSystem::ILoad(int inode_no)
{
  DiskInode d_inode;
  this->m_BufferManager.bread((char *)&d_inode, INODE_START_ADDR + inode_no * INODE_SIZE, INODE_SIZE);
  Inode inode;
  memcpy_s(&inode, INODE_SIZE, &d_inode, INODE_SIZE);
  inode.i_no = inode_no;
  return inode;
}

void FileSystem::ISave(Inode inode)
{
  DiskInode d_inode;
  memcpy_s(&d_inode, INODE_SIZE, &inode, INODE_SIZE);
  this->m_BufferManager.bwrite((const char *)&d_inode, INODE_START_ADDR + inode.i_no * INODE_SIZE, INODE_SIZE);
}

Inode FileSystem::IAlloc()
{
  if (this->spb.s_finode <= 0)
    throw std::runtime_error("Inode申请失败，无空闲的Inode");

  if (this->spb.s_ninode <= 0)
  {
    for (int i = 0; i < INODE_NUM && this->spb.s_ninode < 100; i++)
    {
      Inode tmp = ILoad(i);
      if (!(tmp.i_mode & Inode::IALLOC))
        this->spb.s_inode[this->spb.s_ninode++] = i; // 加入直接管理队列
    }
  }

  if (this->spb.s_ninode <= 0 || this->spb.s_ninode > 100)
    throw std::runtime_error("读取spb.s_ninode越界");

  int res_no = this->spb.s_inode[--this->spb.s_ninode];
  return ILoad(res_no);
}

void FileSystem::IFree(Inode inode)
{
  if (this->spb.s_ninode < 100)
    this->spb.s_inode[spb.s_ninode++] = inode.i_no;

  memset(&inode, 0, INODE_SIZE);
  ISave(inode);
  this->spb.s_finode++;
}

int FileSystem::BAlloc()
{
  if (this->spb.s_fblock <= 0)
    throw std::runtime_error("block申请失败，无空闲的block");

  if (this->spb.s_nfree <= 0 || this->spb.s_nfree > 100)
    throw std::runtime_error("读取spb.s_nfree越界");

  this->spb.s_fblock--;
  int no = this->spb.s_free[--this->spb.s_nfree];

  if (spb.s_nfree == 0 && spb.s_fblock > 0)
  {
    int addr = DATA_START_ADDR + spb.s_free[0] * BLOCK_SIZE;
    this->m_BufferManager.bread((char *)&spb.s_nfree, addr, sizeof(spb.s_nfree));
    this->m_BufferManager.bread((char *)spb.s_free, addr + sizeof(spb.s_nfree), sizeof(spb.s_free));
  }

  return no;
}

void FileSystem::BFree(int blkno)
{
  this->spb.s_fblock++;
  if (spb.s_nfree == 100)
  {
    int addr = DATA_START_ADDR + blkno * BLOCK_SIZE;
    this->m_BufferManager.bwrite((const char *)&spb.s_nfree, addr, sizeof(spb.s_nfree));
    this->m_BufferManager.bwrite((const char *)spb.s_free, addr + int(sizeof(spb.s_nfree)), sizeof(spb.s_free));
    this->spb.s_nfree = 0;
    memset(spb.s_free, 0, sizeof(spb.s_free));
  }
  spb.s_free[spb.s_nfree++] = blkno;
}
