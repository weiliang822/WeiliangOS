#include "ToolFun.h"

/*这里的str不能用引用，原值会被修改*/
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