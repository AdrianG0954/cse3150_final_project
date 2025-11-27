#pragma once
#include <string>
#include <vector>

using std::vector, std::string;

class Utils
{
public:
    static vector<string> split(const string &s, const char delimeter);

    template <typename T>
    static string join(const vector<T> &input, const string &separator);
};
