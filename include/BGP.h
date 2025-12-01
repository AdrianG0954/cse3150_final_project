#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <queue>

#include "Announcement.h"
#include "Policy.h"

using std::string, std::unordered_map, std::queue;

class BGP : public Policy
{
protected:
    int ownerAsn;
    unordered_map<std::string, Announcement> localRib; // routing information table
    queue<Announcement> receivedAnnouncements;         // contains all received announcements to be processed

public:
    BGP(int asn)
    {
        this->ownerAsn = asn;
    }

    const int getOwnerAsn() const
    {
        return ownerAsn;
    }

    void enqueueAnnouncement(const Announcement &announcement) override;

    void processAnnouncements() override;

    const unordered_map<string, Announcement> &getlocalRib() const override
    {
        return localRib;
    }

    Announcement resolveAnnouncement(const Announcement &curr);
};
