#include <string>
#include <vector>
#include <unordered_map>
#include <queue>

#include "ROV.h"
#include "BGP.h"

using std::string, std::vector, std::unordered_map, std::queue;

void ROV::processAnnouncements()
{
    /*
    this is the ROV implementation of processing announcements
    we override the BGP processAnnouncements to drop any announcements
    that are marked as ROV invalid
    */
    while (!receivedAnnouncements.empty())
    {
        Announcement curr = receivedAnnouncements.front();
        string prefix = curr.getPrefix();
        vector<int> newAsPath = curr.getAsPath();
        newAsPath.insert(newAsPath.begin(), this->ownerAsn);
        curr.setAsPath(newAsPath);

        if (!curr.isRovInvalid())
        {
            Announcement toStore = resolveAnnouncement(curr);
            localRib[prefix] = toStore;
        }
        receivedAnnouncements.pop();
    }
}