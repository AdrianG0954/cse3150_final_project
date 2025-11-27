#include <iostream>
#include <string>
#include <fstream>
#include <vector>
#include <unordered_map>

#include "Utils.h"
#include "BGP.h"
#include "Announcement.h"

using std::string, std::vector, std::unordered_map;

void BGP::enqueueAnnouncement(const Announcement &a) 
{
    receivedAnnouncements.push(a);
}

void BGP::clearAnnouncements()
{
    for (size_t i = 0; i < receivedAnnouncements.size(); ++i)
    {
        receivedAnnouncements.pop();
    }
}

void BGP::processAnnouncements()
{
    // for now just add to local rib
    Announcement a = receivedAnnouncements.front();
    receivedAnnouncements.pop();

    localRib[a.getPrefix()] = a;
}

const unordered_map<string, Announcement> &BGP::getlocalRib() const
{
    return localRib;
}
