#include "Utils.h"
#include <vector>
#include <string>
#include <sstream>

using std::string, std::vector, std::ostringstream;

vector<string> Utils::split(const string &s, const char delimiter)
{
    vector<string> res;
    res.reserve(4); // lines have 4 tokens (src|dst|rel|data)

    size_t start = 0, posEnd = 0;
    while ((posEnd = s.find(delimiter, start)) != string::npos)
    {
        res.push_back(s.substr(start, posEnd - start));
        start = posEnd + 1;
    }
    if (start < s.size())
        res.push_back(s.substr(start));

    return res;
}
