#include <iostream>
#include <string>
#include <fstream>
#include <vector>
#include <unordered_map>

#include "Utils.h"
#include "BGP.h"
#include "Announcement.h"
#include "Relationships.h"

using std::string, std::vector, std::unordered_map;

void BGP::enqueueAnnouncement(const Announcement &a)
{
    receivedAnnouncements.push(a);
}

void BGP::enqueueAnnouncement(Announcement &&a)
{
    receivedAnnouncements.push(std::move(a));
}

void BGP::processAnnouncements()
{
    /*
    we loop through all received announcements in the queue
    and create an announcement for them and store it in the localRib

    we should handle conflicts if they arise.
    */
    unordered_map<string, vector<Announcement>> candidatesByPrefix;
    candidatesByPrefix.reserve(receivedAnnouncements.size());

    while (!receivedAnnouncements.empty())
    {
        const string &prefix = receivedAnnouncements.front().getPrefix();
        candidatesByPrefix[prefix].push_back(std::move(receivedAnnouncements.front()));
        receivedAnnouncements.pop();
    }

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

        // Check for existing announcement in localRib
        auto it = localRib.find(prefix);
        if (it != localRib.end())
        {
            Announcement *winner = chooseBest(&it->second, bestNewAnn);
            // if the winner is the existing one, skip updating
            if (winner == &it->second)
                continue;
        }

        // modify AS path after choosing best
        vector<int> &newAsPath = bestNewAnn->getAsPath();
        newAsPath.insert(newAsPath.begin(), this->ownerAsn);
        localRib[prefix] = std::move(*bestNewAnn);
    }
}

Announcement *BGP::chooseBest(Announcement *curr, Announcement *cand)
{
    if (curr == nullptr)
        return cand;

    Relationship currPriority = curr->getRelationship();
    Relationship candPriority = cand->getRelationship();

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
