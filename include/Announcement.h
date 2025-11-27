#pragma once
#include <string>
#include <vector>
#include <memory>

using std::string;

class Announcement
{
private:
    std::string prefix;       // the IP prefix being announced
    std::vector<int> asPath;  // path the announcement has taken up to the current point (pre-pendeded)
    int nextHopAsn;           // the AS that sent this announcement to us
    std::string relationship; // the relationship of the AS that sent the announcement

public:
    Announcement(const std::string &prefix, const std::vector<int> &asPath, int nextHopAsn, const std::string &relationship)
    {
        this->prefix = prefix;
        this->asPath = asPath;
        this->nextHopAsn = nextHopAsn;
        this->relationship = relationship;
    }

    const string &getPrefix() const
    {
        return prefix;
    }

    void setNextHopAsn(int newNextHopAsn)
    {
        this->nextHopAsn = newNextHopAsn;
    }

    const vector<int> &getAsPath() const
    {
        return this->asPath;
    }
};
