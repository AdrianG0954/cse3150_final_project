#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <queue>

#include "Announcement.h"
#include "Policy.h"

using std::string, std::unordered_map;

class BGP : public Policy
{
private:
    std::unordered_map<std::string, Announcement> localRib; // routing information table
    std::queue<Announcement> receivedAnnouncements;         // contains all received announcements to be processed
public:
    BGP() = default;

    void enqueueAnnouncement(const Announcement &announcement) override;

    void processAnnouncements();

    void clearAnnouncements() override;

    const unordered_map<string, Announcement> &getlocalRib() const;
};
