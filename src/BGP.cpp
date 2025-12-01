#include <iostream>
#include <string>
#include <fstream>
#include <vector>
#include <unordered_map>

#include "Utils.h"
#include "BGP.h"
#include "Announcement.h"

using std::string, std::vector, std::unordered_map;

// Helper function to get priority of relationship
static int getPriority(const string &rel)
{
    if (rel == "origin" || rel == "received_from_origin")
        return 4;
    if (rel == "customer" || rel == "received_from_customer")
        return 3;
    if (rel == "peer" || rel == "received_from_peer")
        return 2;
    if (rel == "provider" || rel == "received_from_provider")
        return 1;
    return -1;
}

void BGP::enqueueAnnouncement(const Announcement &a)
{
    receivedAnnouncements.push(a);
}

void BGP::processAnnouncements()
{
    /*
    we loop through all received announcements in the queue
    and create an announcement for them and store it in the localRib

    we should handle conflicts if they arise.
    */
    while (!receivedAnnouncements.empty())
    {
        Announcement curr = receivedAnnouncements.front();
        string prefix = curr.getPrefix();
        vector<int> newAsPath = curr.getAsPath();
        newAsPath.insert(newAsPath.begin(), this->ownerAsn);
        curr.setAsPath(newAsPath);

        Announcement toStore = resolveAnnouncement(curr);

        localRib[prefix] = toStore;
        receivedAnnouncements.pop();
    }
}

Announcement BGP::resolveAnnouncement(const Announcement &newAnn)
{
    /*
    Conflict Resolution:

    When receiving a prefix that already exists,
    we keep the one with the following criteria (respectively):

    - preferred relationship
        - Origin > Customers > Peers > Providers
    - Shortest AS path
    - Lowest next hop ASN
    */
    const string &prefix = newAnn.getPrefix();
    int nextHopAsn = newAnn.getNextHopAsn();
    vector<int> asPath = newAnn.getAsPath();
    const string &relationship = newAnn.getRelationship();

    if (localRib.find(prefix) != localRib.end())
    {
        Announcement &old = localRib[prefix];

        int newPriority = getPriority(newAnn.getRelationship());
        int oldPriority = getPriority(old.getRelationship());
        if (oldPriority > newPriority ||
            (oldPriority == newPriority && old.getAsPath().size() < newAnn.getAsPath().size()) ||
            (oldPriority == newPriority && old.getAsPath().size() == newAnn.getAsPath().size() && old.getNextHopAsn() < newAnn.getNextHopAsn()))
        {
            return old;
        }
    }
    return Announcement(
        prefix,
        asPath,
        nextHopAsn,
        "received_from_" + relationship);
}
