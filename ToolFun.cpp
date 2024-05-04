#include "ToolFun.h"

std::vector<std::string> splitstring(std::string str, const std::string &sep)
{
    std::vector<std::string> res;
    char *pTmp = NULL;
    char *token = strtok_s((char *)str.c_str(), sep.c_str(), &pTmp);

    while (token != NULL)
    {
        res.push_back(std::string(token));
        token = strtok_s(NULL, sep.c_str(), &pTmp);
    }
    return res;
}

std::string findPath(const std::string &path, const std::string &cur_dir)
{
    if (path[0] == '/')
        return path;
    else
        return cur_dir + path;
}

bool argsCheck(const std::string &cmd, int sz, int num)
{
    if (sz < num)
    {
        std::cout << " " << cmd << " 命令参数太少\n";
        return false;
    }
    else if (sz > num)
    {
        std::cout << " " << cmd << " 命令参数太少\n";
        return false;
    }
    return true;
}

void inputPwd(char *password)
{
    std::cout << "password: ";
    char ch = '\0';
    int pos = 0;
    while ((ch = _getch()) != '\r' && pos < 255)
    {
        if (ch != 8) // 不是回撤就录入
        {
            password[pos] = ch;
            putchar('*'); // 并且输出*号
            pos++;
        }
        else
        {
            putchar('\b'); // 这里是删除一个，我们通过输出回撤符 \b，回撤一格，
            putchar(' ');  // 再显示空格符把刚才的*给盖住，
            putchar('\b'); // 然后再 回撤一格等待录入。
            pos--;
        }
    }
    password[pos] = '\0';
    putchar('\n');
}