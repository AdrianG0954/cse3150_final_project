#pragma once
#include <string>
#include <vector>
#include <memory>
#include <unordered_map>

#include "Announcement.h"

class Policy
{
private:
public:
    virtual ~Policy() = default;

    virtual void enqueueAnnouncement(const Announcement &a) = 0;

    virtual void processAnnouncements() = 0;

    virtual void addOrigin(const Announcement &a) = 0;

    virtual const std::unordered_map<std::string, Announcement> &getlocalRib() const = 0;
};
