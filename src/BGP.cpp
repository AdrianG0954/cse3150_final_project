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
    if (rel == "origin")
        return 4;
    if (rel == "customer")
        return 3;
    if (rel == "peer")
        return 2;
    if (rel == "provider")
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
    unordered_map<string, vector<Announcement>> candidatesByPrefix;
    while (!receivedAnnouncements.empty())
    {
        Announcement curr = receivedAnnouncements.front();
        // string prefix = curr.getPrefix();
        // vector<int> newAsPath = curr.getAsPath();
        // newAsPath.insert(newAsPath.begin(), this->ownerAsn);
        // curr.setAsPath(newAsPath);
        receivedAnnouncements.pop();

        // loop prevention, skip if own ASN is already in AS path
        // check before modifying AS path
        bool hasLoop = false;
        const vector<int> &currAsPath = curr.getAsPath();
        for (size_t i = 0; i < currAsPath.size(); ++i)
        {
            if (currAsPath[i] == this->ownerAsn)
            {
                hasLoop = true;
                break;
            }
        }
        if (hasLoop)
            continue;

        candidatesByPrefix[curr.getPrefix()].push_back(curr);
    }
    // Announcement toStore = resolveAnnouncement(curr);

    // localRib[prefix] = toStore;
    // receivedAnnouncements.pop();

    for (auto &pair : candidatesByPrefix)
    {
        const string &prefix = pair.first;
        vector<Announcement> &candidates = pair.second;

        if (candidates.empty())
            continue;

        Announcement *bestNewAnn = &candidates[0];
        for (size_t i = 1; i < candidates.size(); ++i)
        {
            bestNewAnn = chooseBest(bestNewAnn, &candidates[i]);
        }

        if (localRib.find(prefix) != localRib.end())
        {
            Announcement *winner = chooseBest(&localRib[prefix], bestNewAnn);
            if (winner == &localRib[prefix])
                continue;
        }

        Announcement stored = *bestNewAnn;
        vector<int> newAsPath = stored.getAsPath();
        newAsPath.insert(newAsPath.begin(), this->ownerAsn);
        stored.setAsPath(newAsPath);
        localRib[prefix] = stored;
    }
}

Announcement *BGP::chooseBest(Announcement *curr, Announcement *cand)
{
    if (curr == nullptr)
        return cand;

    int currPriority = getPriority(curr->getRelationship());
    int candPriority = getPriority(cand->getRelationship());

    if (candPriority > currPriority)
        return cand;

    if (candPriority < currPriority)
        return curr;

    if (cand->getAsPath().size() < curr->getAsPath().size())
        return cand;

    if (cand->getAsPath().size() > curr->getAsPath().size())
        return curr;

    if (cand->getNextHopAsn() < curr->getNextHopAsn())
        return cand;

    return curr;
}
