#pragma once

#include <string>
#include <vector>
#include <memory>

#include "Relationships.h"

using std::string, std::vector, std::ostream;

class Announcement
{
private:
    string prefix;             // the IP prefix being announced
    vector<int> asPath;        // path the announcement has taken up to the current point (pre-pendeded)
    int nextHopAsn;            // the AS that sent this announcement to us
    Relationship relationship; // the relationship of the AS that sent the announcement
    bool rovInvalid;           // whether the announcement failed ROV checks
public:
    Announcement() = default;

    Announcement(const string &prefix, const vector<int> &asPath, int nextHopAsn, Relationship relationship, bool rovInvalid = false)
    {
        this->prefix = prefix;
        this->asPath = asPath;
        this->nextHopAsn = nextHopAsn;
        this->relationship = relationship;
        this->rovInvalid = rovInvalid;
    }

    const string &getPrefix() const
    {
        return prefix;
    }

    const Relationship &getRelationship() const
    {
        return this->relationship;
    }

    bool isRovInvalid() const
    {
        return this->rovInvalid;
    }

    const vector<int> &getAsPath() const
    {
        return this->asPath;
    }

    vector<int> &getAsPath()
    {
        return this->asPath;
    }

    const void setAsPath(const vector<int> &newAsPath)
    {
        this->asPath = newAsPath;
    }

    int getNextHopAsn() const
    {
        return this->nextHopAsn;
    }

    void setNextHopAsn(int newNextHopAsn)
    {
        this->nextHopAsn = newNextHopAsn;
    }
};
