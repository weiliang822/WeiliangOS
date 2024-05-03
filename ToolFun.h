#include <vector>
#include <string>

/*这里的str不能用引用，原值会被修改*/
std::vector<std::string> splitstring(std::string str, const std::string &sep);