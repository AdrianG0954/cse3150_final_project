#include <gtest/gtest.h>
#include "BGP.h"
#include "Announcement.h"
#include <vector>

class BGPTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        bgp = new BGP(100); // Create BGP instance with ASN 100
    }

    void TearDown() override
    {
        delete bgp;
    }

    BGP *bgp;
};

// ==================== BASIC FUNCTIONALITY TESTS ====================

TEST_F(BGPTest, ConstructorSetsOwnerAsn)
{
    EXPECT_EQ(100, bgp->getOwnerAsn());
}

TEST_F(BGPTest, InitiallyEmptyLocalRib)
{
    const auto &rib = bgp->getlocalRib();
    EXPECT_EQ(0, rib.size());
}

TEST_F(BGPTest, EnqueueSingleAnnouncement)
{
    Announcement ann("192.168.1.0/24", {200}, 200, "customer");
    bgp->enqueueAnnouncement(ann);
    
    // localRib should still be empty (not processed yet)
    EXPECT_EQ(0, bgp->getlocalRib().size());
}

// ==================== PREPENDING TESTS ====================

TEST_F(BGPTest, ProcessAnnouncementPrependsOwnerAsn)
{
    Announcement ann("192.168.1.0/24", {200}, 200, "customer");
    bgp->enqueueAnnouncement(ann);
    bgp->processAnnouncements();
    
    const auto &rib = bgp->getlocalRib();
    ASSERT_EQ(1, rib.size());
    
    const Announcement &stored = rib.at("192.168.1.0/24");
    const auto &asPath = stored.getAsPath();
    
    // AS path should be [100, 200] (owner ASN prepended)
    ASSERT_EQ(2, asPath.size());
    EXPECT_EQ(100, asPath[0]);
    EXPECT_EQ(200, asPath[1]);
}

TEST_F(BGPTest, PrependingMultipleHops)
{
    // Announcement that already went through multiple ASes
    Announcement ann("10.0.0.0/8", {200, 300, 400}, 200, "peer");
    bgp->enqueueAnnouncement(ann);
    bgp->processAnnouncements();
    
    const auto &rib = bgp->getlocalRib();
    const Announcement &stored = rib.at("10.0.0.0/8");
    const auto &asPath = stored.getAsPath();
    
    // AS path should be [100, 200, 300, 400]
    ASSERT_EQ(4, asPath.size());
    EXPECT_EQ(100, asPath[0]);
    EXPECT_EQ(200, asPath[1]);
    EXPECT_EQ(300, asPath[2]);
    EXPECT_EQ(400, asPath[3]);
}

TEST_F(BGPTest, RelationshipPrefixAdded)
{
    Announcement ann("192.168.1.0/24", {200}, 200, "customer");
    bgp->enqueueAnnouncement(ann);
    bgp->processAnnouncements();
    
    const auto &rib = bgp->getlocalRib();
    const Announcement &stored = rib.at("192.168.1.0/24");
    
    EXPECT_EQ("customer", stored.getRelationship());
}

// ==================== CONFLICT RESOLUTION - RELATIONSHIP PRIORITY ====================

TEST_F(BGPTest, ConflictResolution_CustomerBeatsProvider)
{
    // First announcement from provider
    Announcement providerAnn("192.168.1.0/24", {200}, 200, "provider");
    bgp->enqueueAnnouncement(providerAnn);
    bgp->processAnnouncements();
    
    // Second announcement from customer (should win)
    Announcement customerAnn("192.168.1.0/24", {300}, 300, "customer");
    bgp->enqueueAnnouncement(customerAnn);
    bgp->processAnnouncements();
    
    const auto &rib = bgp->getlocalRib();
    ASSERT_EQ(1, rib.size());
    
    const Announcement &stored = rib.at("192.168.1.0/24");
    EXPECT_EQ("customer", stored.getRelationship());
    EXPECT_EQ(300, stored.getNextHopAsn());
}

TEST_F(BGPTest, ConflictResolution_PeerBeatsProvider)
{
    // First announcement from provider
    Announcement providerAnn("192.168.1.0/24", {200}, 200, "provider");
    bgp->enqueueAnnouncement(providerAnn);
    bgp->processAnnouncements();
    
    // Second announcement from peer (should win)
    Announcement peerAnn("192.168.1.0/24", {300}, 300, "peer");
    bgp->enqueueAnnouncement(peerAnn);
    bgp->processAnnouncements();
    
    const auto &rib = bgp->getlocalRib();
    const Announcement &stored = rib.at("192.168.1.0/24");
    EXPECT_EQ("peer", stored.getRelationship());
}

TEST_F(BGPTest, ConflictResolution_OriginBeatsAll)
{
    // First announcement from customer
    Announcement customerAnn("192.168.1.0/24", {200}, 200, "customer");
    bgp->enqueueAnnouncement(customerAnn);
    bgp->processAnnouncements();
    
    // Second announcement from origin (should win)
    Announcement originAnn("192.168.1.0/24", {300}, 300, "origin");
    bgp->enqueueAnnouncement(originAnn);
    bgp->processAnnouncements();
    
    const auto &rib = bgp->getlocalRib();
    const Announcement &stored = rib.at("192.168.1.0/24");
    EXPECT_EQ("origin", stored.getRelationship());
}

TEST_F(BGPTest, ConflictResolution_ProviderDoesNotBeatCustomer)
{
    // First announcement from customer
    Announcement customerAnn("192.168.1.0/24", {200}, 200, "customer");
    bgp->enqueueAnnouncement(customerAnn);
    bgp->processAnnouncements();
    
    // Second announcement from provider (should lose)
    Announcement providerAnn("192.168.1.0/24", {300, 400}, 300, "provider");
    bgp->enqueueAnnouncement(providerAnn);
    bgp->processAnnouncements();
    
    const auto &rib = bgp->getlocalRib();
    const Announcement &stored = rib.at("192.168.1.0/24");
    EXPECT_EQ("customer", stored.getRelationship());
    EXPECT_EQ(200, stored.getNextHopAsn());
}

// ==================== CONFLICT RESOLUTION - AS PATH LENGTH ====================

TEST_F(BGPTest, ConflictResolution_SameRelationship_ShorterPathWins)
{
    // First announcement with longer path
    Announcement longPath("192.168.1.0/24", {200, 300, 400}, 200, "customer");
    bgp->enqueueAnnouncement(longPath);
    bgp->processAnnouncements();
    
    // Second announcement with shorter path (should win)
    Announcement shortPath("192.168.1.0/24", {500}, 500, "customer");
    bgp->enqueueAnnouncement(shortPath);
    bgp->processAnnouncements();
    
    const auto &rib = bgp->getlocalRib();
    const Announcement &stored = rib.at("192.168.1.0/24");
    
    // Should have shorter path: [100, 500]
    EXPECT_EQ(2, stored.getAsPath().size());
    EXPECT_EQ(500, stored.getNextHopAsn());
}

TEST_F(BGPTest, ConflictResolution_SameRelationship_LongerPathLoses)
{
    // First announcement with shorter path
    Announcement shortPath("192.168.1.0/24", {200}, 200, "peer");
    bgp->enqueueAnnouncement(shortPath);
    bgp->processAnnouncements();
    
    // Second announcement with longer path (should lose)
    Announcement longPath("192.168.1.0/24", {300, 400, 500}, 300, "peer");
    bgp->enqueueAnnouncement(longPath);
    bgp->processAnnouncements();
    
    const auto &rib = bgp->getlocalRib();
    const Announcement &stored = rib.at("192.168.1.0/24");
    
    // Should keep shorter path: [100, 200]
    EXPECT_EQ(2, stored.getAsPath().size());
    EXPECT_EQ(200, stored.getNextHopAsn());
}

// ==================== CONFLICT RESOLUTION - NEXT HOP ASN ====================

TEST_F(BGPTest, ConflictResolution_SameRelationshipAndPath_LowerNextHopWins)
{
    // First announcement with higher next hop
    Announcement highNextHop("192.168.1.0/24", {300}, 300, "customer");
    bgp->enqueueAnnouncement(highNextHop);
    bgp->processAnnouncements();
    
    // Second announcement with lower next hop (should win)
    Announcement lowNextHop("192.168.1.0/24", {200}, 200, "customer");
    bgp->enqueueAnnouncement(lowNextHop);
    bgp->processAnnouncements();
    
    const auto &rib = bgp->getlocalRib();
    const Announcement &stored = rib.at("192.168.1.0/24");
    EXPECT_EQ(200, stored.getNextHopAsn());
}

TEST_F(BGPTest, ConflictResolution_PathLengthOverridesNextHop)
{
    // First announcement: longer path but lower next hop
    Announcement longPathLowNext("192.168.1.0/24", {100, 200, 300}, 100, "peer");
    bgp->enqueueAnnouncement(longPathLowNext);
    bgp->processAnnouncements();
    
    // Second announcement: shorter path but higher next hop (should win due to path)
    Announcement shortPathHighNext("192.168.1.0/24", {900}, 900, "peer");
    bgp->enqueueAnnouncement(shortPathHighNext);
    bgp->processAnnouncements();
    
    const auto &rib = bgp->getlocalRib();
    const Announcement &stored = rib.at("192.168.1.0/24");
    
    // Shorter path should win even though next hop is higher
    EXPECT_EQ(2, stored.getAsPath().size()); // [100 (owner), 900]
    EXPECT_EQ(900, stored.getNextHopAsn());
}

// ==================== MULTIPLE PREFIX TESTS ====================

TEST_F(BGPTest, MultipleDifferentPrefixes)
{
    Announcement ann1("192.168.1.0/24", {200}, 200, "customer");
    Announcement ann2("10.0.0.0/8", {300}, 300, "peer");
    Announcement ann3("172.16.0.0/12", {400}, 400, "provider");
    
    bgp->enqueueAnnouncement(ann1);
    bgp->enqueueAnnouncement(ann2);
    bgp->enqueueAnnouncement(ann3);
    bgp->processAnnouncements();
    
    const auto &rib = bgp->getlocalRib();
    ASSERT_EQ(3, rib.size());
    
    EXPECT_TRUE(rib.find("192.168.1.0/24") != rib.end());
    EXPECT_TRUE(rib.find("10.0.0.0/8") != rib.end());
    EXPECT_TRUE(rib.find("172.16.0.0/12") != rib.end());
}

TEST_F(BGPTest, BatchProcessingMultiplePrefixes)
{
    // Enqueue multiple announcements, then process all at once
    bgp->enqueueAnnouncement(Announcement("192.168.1.0/24", {200}, 200, "customer"));
    bgp->enqueueAnnouncement(Announcement("10.0.0.0/8", {300}, 300, "peer"));
    bgp->enqueueAnnouncement(Announcement("172.16.0.0/12", {400}, 400, "provider"));
    
    EXPECT_EQ(0, bgp->getlocalRib().size()); // Not processed yet
    
    bgp->processAnnouncements();
    
    EXPECT_EQ(3, bgp->getlocalRib().size()); // All processed
}

// ==================== COMPLEX SCENARIOS ====================

TEST_F(BGPTest, ComplexConflictResolution_MultipleUpdates)
{
    const std::string prefix = "192.168.1.0/24";
    
    // First: provider announcement
    bgp->enqueueAnnouncement(Announcement(prefix, {200}, 200, "provider"));
    bgp->processAnnouncements();
    EXPECT_EQ("provider", bgp->getlocalRib().at(prefix).getRelationship());
    
    // Second: peer announcement (should win - better relationship)
    bgp->enqueueAnnouncement(Announcement(prefix, {300, 400}, 300, "peer"));
    bgp->processAnnouncements();
    EXPECT_EQ("peer", bgp->getlocalRib().at(prefix).getRelationship());
    
    // Third: customer announcement (should win - best relationship)
    bgp->enqueueAnnouncement(Announcement(prefix, {500, 600, 700}, 500, "customer"));
    bgp->processAnnouncements();
    EXPECT_EQ("customer", bgp->getlocalRib().at(prefix).getRelationship());
    
    // Fourth: another peer announcement with shorter path (should lose - worse relationship)
    bgp->enqueueAnnouncement(Announcement(prefix, {800}, 800, "peer"));
    bgp->processAnnouncements();
    EXPECT_EQ("customer", bgp->getlocalRib().at(prefix).getRelationship());
    EXPECT_EQ(500, bgp->getlocalRib().at(prefix).getNextHopAsn());
}

TEST_F(BGPTest, RealisticPropagationScenario)
{
    // Simulate AS 100 receiving announcements from different neighbors
    
    // From customer AS 50 (short path, good relationship)
    bgp->enqueueAnnouncement(Announcement("203.0.113.0/24", {50}, 50, "customer"));
    
    // From peer AS 200 (medium path, medium relationship)
    bgp->enqueueAnnouncement(Announcement("198.51.100.0/24", {200, 250}, 200, "peer"));
    
    // From provider AS 1000 (various paths)
    bgp->enqueueAnnouncement(Announcement("192.0.2.0/24", {1000, 2000, 3000}, 1000, "provider"));
    
    bgp->processAnnouncements();
    
    const auto &rib = bgp->getlocalRib();
    ASSERT_EQ(3, rib.size());
    
    // Verify each announcement was processed correctly
    const Announcement &ann1 = rib.at("203.0.113.0/24");
    EXPECT_EQ(2, ann1.getAsPath().size()); // [100, 50]
    EXPECT_EQ(100, ann1.getAsPath()[0]);
    EXPECT_EQ("customer", ann1.getRelationship());
    
    const Announcement &ann2 = rib.at("198.51.100.0/24");
    EXPECT_EQ(3, ann2.getAsPath().size()); // [100, 200, 250]
    EXPECT_EQ("peer", ann2.getRelationship());
    
    const Announcement &ann3 = rib.at("192.0.2.0/24");
    EXPECT_EQ(4, ann3.getAsPath().size()); // [100, 1000, 2000, 3000]
    EXPECT_EQ("provider", ann3.getRelationship());
}

// ==================== EDGE CASES ====================

TEST_F(BGPTest, EmptyQueue_ProcessDoesNothing)
{
    bgp->processAnnouncements();
    EXPECT_EQ(0, bgp->getlocalRib().size());
}

TEST_F(BGPTest, SingleHopOriginAnnouncement)
{
    // Origin announcement with just the origin ASN
    Announcement ann("192.168.1.0/24", {100}, 100, "origin");
    bgp->enqueueAnnouncement(ann);
    bgp->processAnnouncements();
    
    const auto &rib = bgp->getlocalRib();
    const Announcement &stored = rib.at("192.168.1.0/24");
    
    // Path should be [100, 100] (owner prepended to origin)
    EXPECT_EQ(2, stored.getAsPath().size());
    EXPECT_EQ("origin", stored.getRelationship());
}

TEST_F(BGPTest, IdenticalAnnouncementsTwice)
{
    Announcement ann("192.168.1.0/24", {200}, 200, "customer");
    
    bgp->enqueueAnnouncement(ann);
    bgp->processAnnouncements();
    
    bgp->enqueueAnnouncement(ann);
    bgp->processAnnouncements();
    
    // Should still only have one entry
    EXPECT_EQ(1, bgp->getlocalRib().size());
    
    // Path should still be correct (new one has same properties, so keeps either)
    const Announcement &stored = bgp->getlocalRib().at("192.168.1.0/24");
    EXPECT_EQ(2, stored.getAsPath().size());
}
