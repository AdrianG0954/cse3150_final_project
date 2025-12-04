#include <gtest/gtest.h>
#include "ROV.h"
#include "Announcement.h"
#include "Relationships.h"
#include <vector>

class ROVTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        rov = new ROV(100); // Create ROV instance with ASN 100
    }

    void TearDown() override
    {
        delete rov;
    }

    ROV *rov;
};

// ==================== ROV FILTERING TESTS ====================

TEST_F(ROVTest, ConstructorSetsOwnerAsn)
{
    EXPECT_EQ(100, rov->getOwnerAsn());
}

TEST_F(ROVTest, AcceptsValidAnnouncement)
{
    Announcement validAnn("192.168.1.0/24", {200}, 200, Relationship::CUSTOMER, false);
    rov->enqueueAnnouncement(validAnn);
    rov->processAnnouncements();

    const auto &rib = rov->getlocalRib();
    EXPECT_EQ(1, rib.size());
    EXPECT_TRUE(rib.find("192.168.1.0/24") != rib.end());
}

TEST_F(ROVTest, RejectsInvalidAnnouncement)
{
    Announcement invalidAnn("192.168.1.0/24", {200}, 200, Relationship::CUSTOMER, true);
    rov->enqueueAnnouncement(invalidAnn);
    rov->processAnnouncements();

    const auto &rib = rov->getlocalRib();
    EXPECT_EQ(0, rib.size()); // Invalid announcement should be rejected
}

TEST_F(ROVTest, RejectsInvalidOrigin)
{
    Announcement invalidOrigin("10.0.0.0/8", {}, 300, Relationship::ORIGIN, true);
    rov->addOrigin(invalidOrigin);

    const auto &rib = rov->getlocalRib();
    EXPECT_EQ(0, rib.size()); // Invalid origin should be rejected
}

TEST_F(ROVTest, AcceptsValidOrigin)
{
    Announcement validOrigin("10.0.0.0/8", {}, 300, Relationship::ORIGIN, false);
    rov->addOrigin(validOrigin);

    const auto &rib = rov->getlocalRib();
    EXPECT_EQ(1, rib.size());
    EXPECT_TRUE(rib.find("10.0.0.0/8") != rib.end());
}

TEST_F(ROVTest, MixedValidInvalidAnnouncements)
{
    // Valid announcement
    Announcement valid1("192.168.1.0/24", {200}, 200, Relationship::CUSTOMER, false);
    rov->enqueueAnnouncement(valid1);

    // Invalid announcement
    Announcement invalid("10.0.0.0/8", {300}, 300, Relationship::PEER, true);
    rov->enqueueAnnouncement(invalid);

    // Another valid announcement
    Announcement valid2("172.16.0.0/12", {400}, 400, Relationship::PROVIDER, false);
    rov->enqueueAnnouncement(valid2);

    rov->processAnnouncements();

    const auto &rib = rov->getlocalRib();
    EXPECT_EQ(2, rib.size()); // Only valid announcements should be in RIB
    EXPECT_TRUE(rib.find("192.168.1.0/24") != rib.end());
    EXPECT_TRUE(rib.find("172.16.0.0/12") != rib.end());
    EXPECT_TRUE(rib.find("10.0.0.0/8") == rib.end()); // Invalid one rejected
}

TEST_F(ROVTest, ValidAnnouncementOverridesExisting)
{
    // First announcement from provider
    Announcement providerAnn("192.168.1.0/24", {200}, 200, Relationship::PROVIDER, false);
    rov->enqueueAnnouncement(providerAnn);
    rov->processAnnouncements();

    // Better announcement from customer should override
    Announcement customerAnn("192.168.1.0/24", {300}, 300, Relationship::CUSTOMER, false);
    rov->enqueueAnnouncement(customerAnn);
    rov->processAnnouncements();

    const auto &rib = rov->getlocalRib();
    EXPECT_EQ(1, rib.size());
    const Announcement &stored = rib.at("192.168.1.0/24");
    EXPECT_EQ(Relationship::CUSTOMER, stored.getRelationship());
    EXPECT_EQ(300, stored.getNextHopAsn());
}

TEST_F(ROVTest, InvalidAnnouncementDoesNotOverrideValid)
{
    // Valid announcement first
    Announcement validAnn("192.168.1.0/24", {200}, 200, Relationship::CUSTOMER, false);
    rov->enqueueAnnouncement(validAnn);
    rov->processAnnouncements();

    // Invalid announcement with better relationship should not override
    Announcement invalidAnn("192.168.1.0/24", {300}, 300, Relationship::ORIGIN, true);
    rov->enqueueAnnouncement(invalidAnn);
    rov->processAnnouncements();

    const auto &rib = rov->getlocalRib();
    EXPECT_EQ(1, rib.size());
    const Announcement &stored = rib.at("192.168.1.0/24");
    EXPECT_EQ(Relationship::CUSTOMER, stored.getRelationship());
    EXPECT_EQ(200, stored.getNextHopAsn()); // Original should still be there
}

// ==================== ROV MOVE SEMANTICS TESTS ====================

TEST_F(ROVTest, MoveSemantics_ValidAnnouncement)
{
    std::string prefix = "192.168.1.0/24";
    std::vector<int> path = {200};

    Announcement ann(std::move(prefix), std::move(path), 200, Relationship::CUSTOMER, false);
    rov->enqueueAnnouncement(std::move(ann));
    rov->processAnnouncements();

    const auto &rib = rov->getlocalRib();
    EXPECT_EQ(1, rib.size());
}

TEST_F(ROVTest, MoveSemantics_InvalidAnnouncement)
{
    std::string prefix = "192.168.1.0/24";
    std::vector<int> path = {200};

    Announcement ann(std::move(prefix), std::move(path), 200, Relationship::CUSTOMER, true);
    rov->enqueueAnnouncement(std::move(ann));
    rov->processAnnouncements();

    const auto &rib = rov->getlocalRib();
    EXPECT_EQ(0, rib.size()); // Invalid should be rejected even with move semantics
}

TEST_F(ROVTest, MultipleInvalidAnnouncementsDifferentPrefixes)
{
    Announcement invalid1("192.168.1.0/24", {200}, 200, Relationship::CUSTOMER, true);
    Announcement invalid2("10.0.0.0/8", {300}, 300, Relationship::PEER, true);
    Announcement invalid3("172.16.0.0/12", {400}, 400, Relationship::PROVIDER, true);

    rov->enqueueAnnouncement(invalid1);
    rov->enqueueAnnouncement(invalid2);
    rov->enqueueAnnouncement(invalid3);
    rov->processAnnouncements();

    const auto &rib = rov->getlocalRib();
    EXPECT_EQ(0, rib.size()); // All should be rejected
}
