#include <gtest/gtest.h>
#include "Announcement.h"
#include "Relationships.h"
#include <vector>
#include <string>

class AnnouncementTest : public ::testing::Test
{
protected:
    void SetUp() override {}
    void TearDown() override {}
};

// ==================== CONSTRUCTOR TESTS ====================

TEST_F(AnnouncementTest, DefaultConstructor)
{
    Announcement ann;
    // Should not crash - default constructor exists
}

TEST_F(AnnouncementTest, CopyConstructor)
{
    std::string prefix = "192.168.1.0/24";
    std::vector<int> path = {100, 200, 300};

    Announcement ann(prefix, path, 200, Relationship::CUSTOMER, false);

    EXPECT_EQ("192.168.1.0/24", ann.getPrefix());
    EXPECT_EQ(3, ann.getAsPath().size());
    EXPECT_EQ(200, ann.getNextHopAsn());
    EXPECT_EQ(Relationship::CUSTOMER, ann.getRelationship());
    EXPECT_FALSE(ann.isRovInvalid());
}

TEST_F(AnnouncementTest, MoveConstructorFromParameters)
{
    std::string prefix = "10.0.0.0/8";
    std::vector<int> path = {500, 600};

    // Move string and vector into announcement
    Announcement ann(std::move(prefix), std::move(path), 500, Relationship::PEER, true);

    EXPECT_EQ("10.0.0.0/8", ann.getPrefix());
    EXPECT_EQ(2, ann.getAsPath().size());
    EXPECT_EQ(500, ann.getAsPath()[0]);
    EXPECT_EQ(600, ann.getAsPath()[1]);
    EXPECT_EQ(500, ann.getNextHopAsn());
    EXPECT_EQ(Relationship::PEER, ann.getRelationship());
    EXPECT_TRUE(ann.isRovInvalid());

    // Note: Copy constructor is called, not move (no move constructor implemented)
    // This is expected behavior with current implementation
}

TEST_F(AnnouncementTest, MoveConstructorFromObject)
{
    Announcement ann1("172.16.0.0/12", {700, 800, 900}, 700, Relationship::PROVIDER, false);

    // Move entire announcement
    Announcement ann2(std::move(ann1));

    EXPECT_EQ("172.16.0.0/12", ann2.getPrefix());
    EXPECT_EQ(3, ann2.getAsPath().size());
    EXPECT_EQ(700, ann2.getNextHopAsn());
    EXPECT_EQ(Relationship::PROVIDER, ann2.getRelationship());
    EXPECT_FALSE(ann2.isRovInvalid());
}

// ==================== GETTER TESTS ====================

TEST_F(AnnouncementTest, GetPrefix)
{
    Announcement ann("192.168.0.0/16", {100}, 100, Relationship::ORIGIN);
    EXPECT_EQ("192.168.0.0/16", ann.getPrefix());
}

TEST_F(AnnouncementTest, GetAsPath)
{
    std::vector<int> path = {100, 200, 300, 400};
    Announcement ann("10.0.0.0/8", path, 100, Relationship::CUSTOMER);

    const std::vector<int> &retrievedPath = ann.getAsPath();
    ASSERT_EQ(4, retrievedPath.size());
    EXPECT_EQ(100, retrievedPath[0]);
    EXPECT_EQ(200, retrievedPath[1]);
    EXPECT_EQ(300, retrievedPath[2]);
    EXPECT_EQ(400, retrievedPath[3]);
}

TEST_F(AnnouncementTest, GetAsPathMutable)
{
    Announcement ann("192.168.1.0/24", {100, 200}, 100, Relationship::PEER);

    // Get mutable reference and modify
    std::vector<int> &path = ann.getAsPath();
    path.push_back(300);

    EXPECT_EQ(3, ann.getAsPath().size());
    EXPECT_EQ(300, ann.getAsPath()[2]);
}

TEST_F(AnnouncementTest, GetNextHopAsn)
{
    Announcement ann("172.16.0.0/12", {500}, 500, Relationship::PROVIDER);
    EXPECT_EQ(500, ann.getNextHopAsn());
}

TEST_F(AnnouncementTest, GetRelationship)
{
    Announcement ann1("10.0.0.0/8", {100}, 100, Relationship::CUSTOMER);
    EXPECT_EQ(Relationship::CUSTOMER, ann1.getRelationship());

    Announcement ann2("10.0.0.0/8", {200}, 200, Relationship::PEER);
    EXPECT_EQ(Relationship::PEER, ann2.getRelationship());

    Announcement ann3("10.0.0.0/8", {300}, 300, Relationship::PROVIDER);
    EXPECT_EQ(Relationship::PROVIDER, ann3.getRelationship());

    Announcement ann4("10.0.0.0/8", {400}, 400, Relationship::ORIGIN);
    EXPECT_EQ(Relationship::ORIGIN, ann4.getRelationship());
}

TEST_F(AnnouncementTest, IsRovInvalid)
{
    Announcement validAnn("192.168.1.0/24", {100}, 100, Relationship::CUSTOMER, false);
    EXPECT_FALSE(validAnn.isRovInvalid());

    Announcement invalidAnn("10.0.0.0/8", {200}, 200, Relationship::PEER, true);
    EXPECT_TRUE(invalidAnn.isRovInvalid());
}

// ==================== SETTER TESTS ====================

TEST_F(AnnouncementTest, SetAsPath)
{
    Announcement ann("192.168.1.0/24", {100}, 100, Relationship::CUSTOMER);

    std::vector<int> newPath = {500, 600, 700};
    ann.setAsPath(newPath);

    const std::vector<int> &retrievedPath = ann.getAsPath();
    ASSERT_EQ(3, retrievedPath.size());
    EXPECT_EQ(500, retrievedPath[0]);
    EXPECT_EQ(600, retrievedPath[1]);
    EXPECT_EQ(700, retrievedPath[2]);
}

TEST_F(AnnouncementTest, SetNextHopAsn)
{
    Announcement ann("10.0.0.0/8", {100}, 100, Relationship::PEER);
    EXPECT_EQ(100, ann.getNextHopAsn());

    ann.setNextHopAsn(999);
    EXPECT_EQ(999, ann.getNextHopAsn());
}

// ==================== EMPTY PATH TESTS ====================

TEST_F(AnnouncementTest, EmptyAsPath)
{
    std::vector<int> emptyPath;
    Announcement ann("192.168.1.0/24", emptyPath, 100, Relationship::ORIGIN);

    EXPECT_EQ(0, ann.getAsPath().size());
}

// ==================== LARGE PATH TESTS ====================

TEST_F(AnnouncementTest, LargeAsPath)
{
    std::vector<int> largePath;
    for (int i = 0; i < 100; ++i)
    {
        largePath.push_back(i);
    }

    Announcement ann("10.0.0.0/8", largePath, 0, Relationship::CUSTOMER);

    EXPECT_EQ(100, ann.getAsPath().size());
    EXPECT_EQ(0, ann.getAsPath()[0]);
    EXPECT_EQ(99, ann.getAsPath()[99]);
}

// ==================== RELATIONSHIP ENUM TESTS ====================

TEST_F(AnnouncementTest, AllRelationshipTypes)
{
    Announcement origin("10.0.0.0/8", {}, 100, Relationship::ORIGIN);
    Announcement customer("192.168.1.0/24", {200}, 200, Relationship::CUSTOMER);
    Announcement peer("172.16.0.0/12", {300}, 300, Relationship::PEER);
    Announcement provider("8.8.8.0/24", {400}, 400, Relationship::PROVIDER);

    EXPECT_EQ(Relationship::ORIGIN, origin.getRelationship());
    EXPECT_EQ(Relationship::CUSTOMER, customer.getRelationship());
    EXPECT_EQ(Relationship::PEER, peer.getRelationship());
    EXPECT_EQ(Relationship::PROVIDER, provider.getRelationship());
}

// ==================== COPY VS MOVE SEMANTICS ====================

TEST_F(AnnouncementTest, CopyVsMoveBehavior)
{
    std::string prefix1 = "192.168.1.0/24";
    std::vector<int> path1 = {100, 200};

    // Copy construction
    Announcement copyAnn(prefix1, path1, 100, Relationship::CUSTOMER);
    EXPECT_FALSE(prefix1.empty()); // Original should still be intact
    EXPECT_EQ(2, path1.size());    // Original should still be intact

    std::string prefix2 = "10.0.0.0/8";
    std::vector<int> path2 = {300, 400};

    // Even with std::move, copy constructor is called (no move constructor implemented)
    Announcement moveAnn(std::move(prefix2), std::move(path2), 300, Relationship::PEER);
    // Originals are still intact since copy constructor was used
    EXPECT_FALSE(prefix2.empty());
    EXPECT_FALSE(path2.empty());
}

TEST_F(AnnouncementTest, MultipleSetters)
{
    Announcement ann("192.168.1.0/24", {100}, 100, Relationship::CUSTOMER);

    // Modify using setters
    ann.setNextHopAsn(500);
    ann.setAsPath({600, 700, 800});

    EXPECT_EQ(500, ann.getNextHopAsn());
    EXPECT_EQ(3, ann.getAsPath().size());
    EXPECT_EQ(600, ann.getAsPath()[0]);
    EXPECT_EQ(800, ann.getAsPath()[2]);
}
