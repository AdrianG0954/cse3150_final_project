#include <string>
#include <vector>
#include <unordered_map>
#include <queue>
#include <iostream>

#include "ROV.h"
#include "BGP.h"

using std::string, std::vector, std::unordered_map, std::queue, std::cout, std::endl;

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
        else
        {
            cout << "Dropping: " << prefix << " at AS " << this->ownerAsn << " due to ROV invalidity." << endl;
        }
        receivedAnnouncements.pop();
    }
}