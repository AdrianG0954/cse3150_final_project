#pragma once
#include <string>
#include <vector>
#include <sstream>

using std::vector, std::string, std::ostringstream;

class Utils
{
public:
    static vector<string> split(const string &s, const char delimeter);

    template <typename T>
    static string join(const vector<T> &input, const string &separator)
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
};
