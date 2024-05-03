#ifndef FILE_SYSTEM_H
#define FILE_SYSTEM_H

#define WLOS_DISK_NAME "Wlos.img"
#define BLOCK_SIZE 512
#define WLOS_DISK_SIZE 256 * 1024 * 1024 // 256MB
#define SUPERBLOCK_START_ADDR 200 * BLOCK_SIZE
#define SUPERBLOCK_SIZE 2 * BLOCK_SIZE
#define INODE_START_ADDR SUPERBLOCK_START_ADDR + SUPERBLOCK_SIZE // 外存inode区起始地址
#define INODE_SIZE 64
#define INODE_ZONE_SIZE (1024 - 202) * BLOCK_SIZE		// 外存inode区的大小
#define INODE_NUM INODE_ZONE_SIZE / INODE_SIZE			// inode个数
#define DATA_START_ADDR 1024 * BLOCK_SIZE				// 数据区起始地址
#define DATA_ZONE_SIZE WLOS_DISK_SIZE - DATA_START_ADDR // 数据区大小
#define ROOT_DIR_INO 0									// 根目录的结点号
#define DIRECTORY_ENTRY_SIZE 32

#include "INode.h"
#include "Buf.h"
#include "BufferManager.h"
// #include "FileManager.h"
#include <fstream>
#include <stdexcept>
#include <time.h>

/* 文件系统存储资源管理块(Super Block)的定义。 */
class SuperBlock
{
public:
	SuperBlock();
	~SuperBlock();

	int s_isize; /* 外存Inode区占用的盘块数 */
	int s_bsize; /* 盘块总数 */

	int s_nfree;	 /* 直接管理的空闲盘块数量 */
	int s_free[100]; /* 直接管理的空闲盘块索引表 */

	int s_ninode;	  /* 直接管理的空闲外存Inode数量 */
	int s_inode[100]; /* 直接管理的空闲外存Inode索引表 */

	int s_flock; /* 封锁空闲盘块索引表标志 */
	int s_ilock; /* 封锁空闲Inode表标志 */

	int s_finode; /*空闲inode的数量*/
	int s_fblock; /*空闲块的数量*/

	int padding[48]; /* 填充使SuperBlock块大小等于1024字节，占据2个扇区 */
};

/* 文件系统类(FileSystem)管理文件存储设备中的各类存储资源，磁盘块、外存INode的分配、释放。*/
class FileSystem
{
public:
	FileSystem();
	~FileSystem();

	/* 格式化文件卷 */
	void fformat();

	/* 初始化成员变量 */
	void Initialize();

	/* 读外存Inode到内存 */
	Inode ILoad(int inode_no);

	/* 存内存inode到外存inode */
	void ISave(Inode inode);

	/* 在存储设备上分配一个空闲外存INode，一般用于创建新的文件。*/
	Inode IAlloc();

	/* 释放外存INode，一般用于删除文件。*/
	void IFree(Inode inode);

	/* 分配空闲磁盘块 */
	int BAlloc();

	/* 释放编号为blkno的磁盘块 */
	void BFree(int blkno);

	// 	friend class FileManager;

	// private:
	SuperBlock spb;
	BufferManager m_BufferManager; /* FileSystem类需要缓存管理模块(BufferManager)提供的接口 */
};

class DirectoryEntry
{
public:
	static const int DIRSIZ = 28; /* 目录项中路径部分的最大字符串长度 */

public:
	DirectoryEntry();
	~DirectoryEntry();

public:
	int m_ino;			 /* 目录项中Inode编号部分 */
	char m_name[DIRSIZ]; /* 目录项中路径名部分 */
};

#endif
