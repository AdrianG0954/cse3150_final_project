#pragma once
#include <string>
#include <vector>
#include <memory>

#include "Announcement.h"

using std::string;

class Policy
{
private:
public:
    virtual ~Policy() = default;

    virtual void enqueueAnnouncement(const Announcement &announcement) = 0;

    virtual void clearAnnouncements() = 0;
};
