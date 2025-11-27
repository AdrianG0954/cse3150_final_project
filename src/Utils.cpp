#include "Utils.h"
#include <vector>
#include <string>

using std::string, std::vector;

vector<string> Utils::split(const string &s, const char delimiter)
{
    std::vector<std::string> res;
    size_t start = 0, posEnd = 0;
    while ((posEnd = s.find(delimiter, start)) != std::string::npos)
    {
        res.push_back(s.substr(start, posEnd - start));
        start = posEnd + 1;
    }
    if (start < s.size())
        res.push_back(s.substr(start));

    return res;
}
