#include "BufferManager.h"
#include <iostream>

BufferManager::BufferManager()
{
  std::cout << "正在初始化缓存管理模块......\n";
  Initialize();
  std::cout << "缓存管理模块初始化完成......\n";
}

BufferManager::~BufferManager()
{
  for (int i = 0; i < NBUF; i++)
    this->Brelease(&m_Buf[i]);
}

void BufferManager::Initialize()
{
  int i;
  Buf *bp;

  // this->bFreeList.b_blkno = -1;
  this->bFreeList.forw = &(this->bFreeList);
  this->bFreeList.back = &(this->bFreeList);
  this->bFreeList.b_addr = NULL;
  this->bFreeList.b_flags = Buf::B_INI;
  for (i = 0; i < NBUF; i++)
  {
    bp = &(this->m_Buf[i]);
    bp->b_addr = this->Buffer[i];
    bp->b_flags = Buf::B_INI;

    /* 初始化自由队列,m_buf[0]位于第一位 */
    bp->back = &(this->bFreeList);
    bp->forw = this->bFreeList.forw;
    this->bFreeList.forw->back = bp;
    this->bFreeList.forw = bp;

    bp->dev_forw = NULL;
    bp->dev_back = NULL;
  }
  // 初始化IO队列
  this->DevList.back = &(this->DevList);
  this->DevList.forw = &(this->DevList);
  // 初始化设备队列
  this->DevList.dev_back = &(this->DevList);
  this->DevList.dev_forw = &(this->DevList);

  this->DevList.b_flags = Buf::B_INI;
  return;
}

Buf *BufferManager::GetBlk(int blkno)
{
  Buf *bp = NULL;

  /*查询是否在设备队列中*/
  for (bp = DevList.dev_back; bp != &DevList; bp = bp->dev_back)
    if (bp->b_blkno == blkno)
      return bp;

  if (bFreeList.back == &bFreeList)
    throw std::runtime_error("缓存自由队列空！");
  else
  {
    bp = bFreeList.back;
    // 从自由队列取出
    bp->forw->back = bp->back;
    bp->back->forw = bp->forw;
    // 从原设备队列取出
    if (bp->dev_forw != NULL && bp->dev_back != NULL)
    {
      bp->dev_forw->dev_back = bp->dev_back;
      bp->dev_back->dev_forw = bp->dev_forw;
    }

    // 有延迟写
    if (bp->b_flags & Buf::BufFlag::B_DELWRI)
    {
      Bwrite(bp);
      bp->b_flags &= ~Buf::BufFlag::B_DELWRI;
    }
    bp->b_flags = Buf::BufFlag::B_INI;
    bp->b_blkno = blkno;
    // 插入设备队列
    bp->dev_forw = &(this->DevList);
    bp->dev_back = this->DevList.dev_back;
    bp->dev_forw->dev_back = bp;
    bp->dev_back->dev_forw = bp;
  }
  return bp;
}

void BufferManager::Brelease(Buf *bp)
{
  // 有延迟写
  if (bp->b_flags & Buf::BufFlag::B_DELWRI)
  {
    Bwrite(bp);
    bp->b_flags &= ~Buf::BufFlag::B_DELWRI;
  }

  bp->b_flags = Buf::BufFlag::B_INI;

  // 取出设备队列
  bp->dev_forw->dev_back = bp->dev_back;
  bp->dev_back->dev_forw = bp->dev_forw;

  // 放入自由队列
  bp->forw->back = bp->back;
  bp->back->forw = bp->forw;

  bp->back = &(this->bFreeList);
  bp->forw = this->bFreeList.forw;
  this->bFreeList.forw->back = bp;
  this->bFreeList.forw = bp;

  return;
}

Buf *BufferManager::Bread(int blkno)
{
  Buf *bp;
  /* 根据字符块号申请缓存 */
  bp = this->GetBlk(blkno);
  /* 如果在设备队列中找到所需缓存，即B_DONE已设置，就不需进行I/O操作 */
  if (bp->b_flags & Buf::BufFlag::B_DONE)
    return bp;
  else
  {
    // 插入IO队列
    bp->forw = this->DevList.forw;
    bp->back = &(this->DevList);
    bp->forw->back = bp;
    bp->back->forw = bp;

    // 开始读
    std::fstream fin;
    fin.open(WLOS_DISK_NAME, std::ios::in | std::ios::binary);
    if (!fin.is_open())
      throw std::runtime_error("磁盘映像文件输入流打开失败！");
    fin.seekg(std::streampos(blkno) * std::streampos(BUFFER_SIZE), std::ios::beg);
    fin.read(bp->b_addr, BUFFER_SIZE);
    fin.close();
    // 结束读
    bp->b_flags = Buf::BufFlag::B_DONE;
    // 从IO队列取出
    bp->forw->back = bp->back;
    bp->back->forw = bp->forw;
    // 加入自由队列
    bp->back = &(this->bFreeList);
    bp->forw = this->bFreeList.forw;
    this->bFreeList.forw->back = bp;
    this->bFreeList.forw = bp;
  }
  return bp;
}

void BufferManager::Bwrite(Buf *bp)
{
  // 插入IO队列
  bp->forw = this->DevList.forw;
  bp->back = &(this->DevList);
  bp->forw->back = bp;
  bp->back->forw = bp;
  // 开始写
  std::fstream fout;
  fout.open(WLOS_DISK_NAME, std::ios::in | std::ios::out | std::ios::binary);
  if (!fout.is_open())
    throw std::runtime_error("磁盘映像文件输出流打开失败\n");

  fout.seekp(std::streampos(bp->b_blkno) * std::streampos(BUFFER_SIZE), std::ios::beg);
  fout.write((const char *)bp->b_addr, BUFFER_SIZE);
  fout.close();
  // 写完成
  bp->b_flags = Buf::BufFlag::B_DONE;
  // 从IO队列取出
  bp->forw->back = bp->back;
  bp->back->forw = bp->forw;
  // 加入自由队列
  bp->back = &(this->bFreeList);
  bp->forw = this->bFreeList.forw;
  this->bFreeList.forw->back = bp;
  this->bFreeList.forw = bp;
  return;
}

void BufferManager::Bdwrite(Buf *bp)
{
  /* 置上B_DONE允许其它进程使用该磁盘块内容 */
  bp->b_flags |= (Buf::BufFlag::B_DELWRI | Buf::BufFlag::B_DONE);
  return;
}

void BufferManager::bread(char *des_addr, unsigned int src_addr, unsigned int len)
{
  if (des_addr == NULL)
    return;
  unsigned int pos = 0;
  unsigned int start_blkno = src_addr / BUFFER_SIZE;
  unsigned int end_blkno = (src_addr + len - 1) / BUFFER_SIZE;
  for (unsigned int blkno = start_blkno; blkno <= end_blkno; blkno++)
  {
    Buf *bp = Bread(blkno);
    if (blkno == start_blkno && blkno == end_blkno)
    {
      memcpy_s(des_addr + pos, len, bp->b_addr + src_addr % BUFFER_SIZE, len);
      pos += len;
    }
    else if (blkno == start_blkno)
    {
      memcpy_s(des_addr + pos, (BUFFER_SIZE - 1) - (src_addr % BUFFER_SIZE) + 1, bp->b_addr + src_addr % BUFFER_SIZE, (BUFFER_SIZE - 1) - (src_addr % BUFFER_SIZE) + 1);
      pos += BUFFER_SIZE - (src_addr % BUFFER_SIZE);
    }
    else if (blkno == end_blkno)
    {
      memcpy_s(des_addr + pos, (src_addr + len - 1) % BUFFER_SIZE - 0 + 1, bp->b_addr, (src_addr + len - 1) % BUFFER_SIZE - 0 + 1);
      pos += (src_addr + len - 1) % BUFFER_SIZE - 0 + 1;
    }
    else
    {
      memcpy_s(des_addr + pos, BUFFER_SIZE, bp->b_addr, BUFFER_SIZE);
      pos += BUFFER_SIZE;
    }
  }
}

void BufferManager::bwrite(const char *src_addr, unsigned int des_addr, unsigned int len)
{
  unsigned int pos = 0;
  unsigned int start_blkno = des_addr / BUFFER_SIZE;
  unsigned int end_blkno = (des_addr + len - 1) / BUFFER_SIZE;
  for (unsigned int blkno = start_blkno; blkno <= end_blkno; blkno++)
  {
    Buf *bp = Bread(blkno);
    if (blkno == start_blkno && blkno == end_blkno)
    {
      memcpy_s(bp->b_addr + des_addr % BUFFER_SIZE, len, src_addr + pos, len);
      pos += len;
    }
    else if (blkno == start_blkno)
    {
      memcpy_s(bp->b_addr + des_addr % BUFFER_SIZE, (BUFFER_SIZE - 1) - (des_addr % BUFFER_SIZE) + 1, src_addr + pos, (BUFFER_SIZE - 1) - (des_addr % BUFFER_SIZE) + 1);
      pos += BUFFER_SIZE - (des_addr % BUFFER_SIZE);
    }
    else if (blkno == end_blkno)
    {
      memcpy_s(bp->b_addr, (des_addr + len - 1) % BUFFER_SIZE - 0 + 1, src_addr + pos, (des_addr + len - 1) % BUFFER_SIZE - 0 + 1);
      pos += (des_addr + len - 1) % BUFFER_SIZE - 0 + 1;
    }
    else
    {
      memcpy_s(bp->b_addr, BUFFER_SIZE, src_addr + pos, BUFFER_SIZE);
      pos += BUFFER_SIZE;
    }
    Bdwrite(bp); // 延迟写
  }
}

void BufferManager::ClrBuf(Buf *bp)
{
  int *pInt = (int *)bp->b_addr;

  /* 将缓冲区中数据清零 */
  for (unsigned int i = 0; i < BufferManager::BUFFER_SIZE / sizeof(int); i++)
  {
    pInt[i] = 0;
  }
  return;
}

Buf &BufferManager::GetBFreeList() { return this->bFreeList; }
