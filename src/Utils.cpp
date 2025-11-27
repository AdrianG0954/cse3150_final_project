#include "Utils.h"
#include <vector>
#include <string>
#include <sstream>

using std::string, std::vector, std::ostringstream;

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

template <typename T>
string Utils::join(const vector<T> &input, const string &separator)
{
    /*
    joins the vector into one string seperated by the specified separator.
    */
    ostringstream oss;
    for (size_t i = 0; i < input.size(); ++i)
    {
        // concat x
        oss << input[i];

        // concat seperator if not at the end
        if (i != input.size() - 1)
            oss << separator;
    }

    return oss.str();
}
