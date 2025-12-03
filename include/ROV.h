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

    void enqueueAnnouncement(const Announcement &a) override
    {
        if (a.isRovInvalid())
        {
            return;
        }
        BGP::enqueueAnnouncement(a);
    }
    
    void enqueueAnnouncement(Announcement &&a)
    {
        if (a.isRovInvalid())
        {
            return;
        }
        BGP::enqueueAnnouncement(std::move(a));
    }

    void processAnnouncements() override
    {
        BGP::processAnnouncements();
    }

    void addOrigin(const Announcement &a) override
    {
        if (a.isRovInvalid())
        {
            return;
        }
        BGP::addOrigin(a);
    }

    const unordered_map<string, Announcement> &getlocalRib() const override
    {
        return BGP::getlocalRib();
    }
};
