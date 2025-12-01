#pragma once

#include <string>
#include <unordered_map>

#include "BGP.h"
#include "Announcement.h"
#include "Policy.h"

using std::string, std::unordered_map;

class ROV : public BGP
{
public:
    ROV(int asn) : BGP(asn) {}

    void enqueueAnnouncement(const Announcement &announcement) override
    {
        BGP::enqueueAnnouncement(announcement);
    }

    void processAnnouncements() override;

    const unordered_map<string, Announcement> &getlocalRib() const override
    {
        return BGP::getlocalRib();
    }
};
