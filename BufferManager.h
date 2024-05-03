#ifndef BUFFER_MANAGER_H
#define BUFFER_MANAGER_H
#define WLOS_DISK_NAME "Wlos.img"

#include <fstream>
#include <stdexcept>
#include "Buf.h"

class BufferManager
{
public:
	static const int NBUF = 15;			/* 缓存控制块、缓冲区的数量 */
	static const int BUFFER_SIZE = 512; /* 缓冲区大小。 以字节为单位 */

public:
	BufferManager();
	~BufferManager();

	void Initialize(); /* 缓存控制块队列的初始化。将缓存控制块中b_addr指向相应缓冲区首地址。*/

	Buf *GetBlk(int blkno); /* 申请一块缓存，用于读写设备上的字符块blkno。*/
	void Brelease(Buf *bp); /* 释放缓存控制块buf */

	Buf *Bread(int blkno); /* 读一个磁盘块。blkno为目标磁盘块逻辑块号。 */
	void Bwrite(Buf *bp);  /* 写一个磁盘块 */
	void Bdwrite(Buf *bp); /* 延迟写磁盘块 */

	void bread(char *des_addr, unsigned int src_addr, unsigned int len);		/*将原始内容通过缓存读入目标地址*/
	void bwrite(const char *src_addr, unsigned int des_addr, unsigned int len); /*将原始内容写入缓存*/

	void ClrBuf(Buf *bp); /* 清空缓冲区内容 */
	Buf &GetBFreeList();  /* 获取自由缓存队列控制块Buf对象引用 */

private:
	Buf bFreeList;	 /* 自由缓存队列控制块 */
	Buf m_Buf[NBUF]; /* 缓存控制块数组 */
	Buf DevList;
	char Buffer[NBUF][BUFFER_SIZE]; /* 缓冲区数组 */
};

#endif
