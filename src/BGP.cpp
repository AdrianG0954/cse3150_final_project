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
    // use move to avoid copying
    receivedAnnouncements.push(std::move(a));
}

void BGP::processAnnouncements()
{
    /*
    we first pop all received announcements in the queue
    and store them as candidates for each prefix

    for each prefix, we choose the best announcement among the candidates
    and compare it with the existing announcement in localRib (if any)

    the overall winnder is stored in localRib
    */
    unordered_map<string, vector<Announcement>> candidates;
    candidates.reserve(receivedAnnouncements.size());

    while (!receivedAnnouncements.empty())
    {
        const string &prefix = receivedAnnouncements.front().getPrefix();
        candidates[prefix].push_back(std::move(receivedAnnouncements.front()));
        receivedAnnouncements.pop();
    }

    for (auto &pair : candidates)
    {
        const string &prefix = pair.first;
        vector<Announcement> &currCandidates = pair.second;

        if (currCandidates.empty())
            continue;

        Announcement *bestNewAnn = &currCandidates[0];
        for (size_t i = 1; i < currCandidates.size(); ++i)
        {
            bestNewAnn = chooseBest(bestNewAnn, &currCandidates[i]);
        }

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
