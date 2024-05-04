#include <vector>
#include <string>
#include <iostream>
#include <conio.h>

/*这里的str不能用引用，原值会被修改*/
std::vector<std::string> splitstring(std::string str, const std::string &sep);
/*寻找绝对路径*/
std::string findPath(const std::string &path, const std::string &cur_dir);
/*检查命令参数，sz是实际参数个数，num是需要个数*/
bool argsCheck(const std::string &cmd, int sz, int num);
/*输入密码*/
void inputPwd(char *password);