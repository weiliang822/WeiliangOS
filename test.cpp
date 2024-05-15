#include <iostream>
#include "FileManager.h"

void test()
{
    FileManager FM;
    FM.dcreate("/test", 0, 0);
    FM.fcreate("/test/Jerry", 0, 0);
    File *fp = FM.fopen("/test/Jerry", 0, 0);
    if (fp == NULL)
    {
        std::cout << "文件打开失败\n";
        return;
    }
    char buf[805] = {0};
    for (int i = 0; i < 800; i++)
    {
        if (i < 500)
            buf[i] = '0' + rand() % 10;
        else
            buf[i] = 'a' + rand() % 26;
    }
    std::cout << "写入内容：" << buf << '\n';
    std::cout << "开始写入...\n";
    FM.fwrite(buf, 800, fp);
    std::cout << "写入完成，当前文件大小为" << fp->f_inode->i_size << "B, 当前文件指针位于：" << FM.ftell(fp) << '\n';
    std::cout << "重定位文件指针\n";
    FM.flseek(fp, 500, SEEK_SET);
    std::cout << "当前文件指针位于：" << FM.ftell(fp) << '\n';
    char abc[505] = {0};
    std::cout << "开始读出...\n";
    FM.fread(abc, 500, fp);
    std::cout << "读出完成，读出内容为: ";
    for (int i = 0; i < 500; i++)
    {
        if (abc[i] >= '0' && abc[i] <= '9' || abc[i] >= 'a' && abc[i] <= 'z')
            std::cout << abc[i];
        else
            std::cout << '?';
    }
    std::cout << '\n';
    std::cout << "当前文件指针位于：" << FM.ftell(fp) << '\n';
    std::cout << "再次写入...\n";
    FM.fwrite(abc, 500, fp);
    std::cout << "写入完成，当前文件大小为" << fp->f_inode->i_size << "B, 当前文件指针位于：" << FM.ftell(fp) << '\n';
    FM.flseek(fp, 0, SEEK_SET);
    char *data = new char[fp->f_inode->i_size + 1]{0};
    FM.fread(data, fp->f_inode->i_size, fp);
    std::cout << "当前文件内容为: ";
    for (int i = 0; i < fp->f_inode->i_size; i++)
    {
        if (data[i] >= '0' && data[i] <= '9' || data[i] >= 'a' && data[i] <= 'z')
            std::cout << data[i];
        else
            std::cout << '?';
    }
    std::cout << '\n';
    FM.fclose(fp);
    std::cout << "自动测试流程完成\n";
}

int main()
{
    srand(time(NULL));
    std::cout << "测试WeiliangOS文件系统\n";
    test();
}