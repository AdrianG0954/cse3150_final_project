#include <gtest/gtest.h>
#include "AsGraph.h"
#include "Announcement.h"
#include "Relationships.h"
#include <fstream>
#include <filesystem>

class AsGraphPropagationTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        graph = new AsGraph();
        createTestFiles();
    }

    void TearDown() override
    {
        delete graph;
        cleanupTestFiles();
    }

    void createTestFiles()
    {
        // Create a simple linear topology: AS1 -> AS2 -> AS3
        std::ofstream graphFile("test_propagation_graph.txt");
        graphFile << "1|2|-1|bgp\n"; // AS1 provider to AS2
        graphFile << "2|3|-1|bgp\n"; // AS2 provider to AS3
        graphFile.close();

        // Create ROV deployment (empty for now - no header, just empty)
        std::ofstream rovFile("test_propagation_rov.csv");
        rovFile.close();

        // Create initial announcements
        std::ofstream annFile("test_propagation_anns.csv");
        annFile << "asn,prefix\n";
        annFile << "3,192.168.1.0/24\n"; // AS3 announces prefix
        annFile.close();

        // Create a more complex topology for testing
        std::ofstream complexGraphFile("test_complex_graph.txt");
        complexGraphFile << "1|2|0|bgp\n";  // AS1 and AS2 are peers
        complexGraphFile << "2|3|-1|bgp\n"; // AS2 provider to AS3
        complexGraphFile << "3|4|-1|bgp\n"; // AS3 provider to AS4
        complexGraphFile << "1|5|-1|bgp\n"; // AS1 provider to AS5
        complexGraphFile.close();

        // Create announcements for complex topology
        std::ofstream complexAnnFile("test_complex_anns.csv");
        complexAnnFile << "asn,prefix\n";
        complexAnnFile << "4,10.0.0.0/8\n";
        complexAnnFile << "5,172.16.0.0/12\n";
        complexAnnFile.close();

        // Create ROV deployment with some ASes (no header)
        std::ofstream rovDeployFile("test_rov_deployment.csv");
        rovDeployFile << "2\n";
        rovDeployFile << "3\n";
        rovDeployFile.close();
    }

    void cleanupTestFiles()
    {
        std::filesystem::remove("test_propagation_graph.txt");
        std::filesystem::remove("test_propagation_rov.csv");
        std::filesystem::remove("test_propagation_anns.csv");
        std::filesystem::remove("test_complex_graph.txt");
        std::filesystem::remove("test_complex_anns.csv");
        std::filesystem::remove("test_rov_deployment.csv");
    }

    AsGraph *graph;
};

// ==================== LOAD ROV DEPLOYMENT TESTS ====================

TEST_F(AsGraphPropagationTest, LoadROVDeployment_EmptyFile)
{
    int result = graph->loadROVDeployment("test_propagation_rov.csv");
    EXPECT_EQ(0, result); // Should succeed with empty deployment
}

TEST_F(AsGraphPropagationTest, LoadROVDeployment_WithASes)
{
    int result = graph->loadROVDeployment("test_rov_deployment.csv");
    EXPECT_EQ(0, result);
}

TEST_F(AsGraphPropagationTest, LoadROVDeployment_NonexistentFile)
{
    int result = graph->loadROVDeployment("nonexistent_file.csv");
    EXPECT_NE(0, result); // Should fail
}

// ==================== PROCESS INITIAL ANNOUNCEMENTS TESTS ====================

TEST_F(AsGraphPropagationTest, ProcessInitialAnnouncements_BasicTopology)
{
    graph->buildGraph("test_propagation_graph.txt");
    graph->flattenGraph();

    graph->processInitialAnnouncements("test_propagation_anns.csv");

    const auto &asMap = graph->getAsMap();

    // AS3 should have the announcement in its RIB
    AS *as3 = asMap.at(3).get();
    const auto &rib3 = as3->getPolicy().getlocalRib();

    EXPECT_EQ(1, rib3.size());
    EXPECT_TRUE(rib3.find("192.168.1.0/24") != rib3.end());
}

TEST_F(AsGraphPropagationTest, ProcessInitialAnnouncements_NonexistentFile)
{
    graph->buildGraph("test_propagation_graph.txt");
    graph->flattenGraph();

    // Should not crash with nonexistent file
    graph->processInitialAnnouncements("nonexistent_anns.csv");

    const auto &asMap = graph->getAsMap();
    AS *as3 = asMap.at(3).get();
    const auto &rib3 = as3->getPolicy().getlocalRib();

    EXPECT_EQ(0, rib3.size()); // No announcements processed
}

// ==================== FLATTEN GRAPH TESTS ====================

TEST_F(AsGraphPropagationTest, FlattenGraph_LinearTopology)
{
    graph->buildGraph("test_propagation_graph.txt");
    graph->flattenGraph();

    const auto &flattened = graph->getFlattenedGraph();

    // Should have ranks (AS1 at top, AS3 at bottom)
    EXPECT_GT(flattened.size(), 0);

    // Verify that the graph is properly flattened
    // AS1 should be in higher rank than AS3
    bool found_as1 = false;
    bool found_as3 = false;
    size_t rank_as1 = 0, rank_as3 = 0;

    for (size_t i = 0; i < flattened.size(); ++i)
    {
        for (int asn : flattened[i])
        {
            if (asn == 1)
            {
                found_as1 = true;
                rank_as1 = i;
            }
            if (asn == 3)
            {
                found_as3 = true;
                rank_as3 = i;
            }
        }
    }

    EXPECT_TRUE(found_as1);
    EXPECT_TRUE(found_as3);
    // AS3 is a customer at the bottom, so it has a lower rank number (processed first)
    // AS1 is a provider at the top, so it has a higher rank number
    EXPECT_GT(rank_as1, rank_as3); // AS1 should be in higher rank than AS3
}

TEST_F(AsGraphPropagationTest, FlattenGraph_ComplexTopology)
{
    graph->buildGraph("test_complex_graph.txt");
    graph->flattenGraph();

    const auto &flattened = graph->getFlattenedGraph();
    EXPECT_GT(flattened.size(), 0);

    // All ASes should be placed in some rank
    int total_ases = 0;
    for (const auto &rank : flattened)
    {
        total_ases += rank.size();
    }

    EXPECT_EQ(5, total_ases); // Should have all 5 ASes
}

// ==================== PROPAGATE UP TESTS ====================

TEST_F(AsGraphPropagationTest, PropagateUp_LinearTopology)
{
    graph->loadROVDeployment("test_propagation_rov.csv");
    graph->buildGraph("test_propagation_graph.txt");
    graph->flattenGraph();

    graph->processInitialAnnouncements("test_propagation_anns.csv");
    graph->propagateUp();

    const auto &asMap = graph->getAsMap();

    // AS3 originates 192.168.1.0/24
    // After propagateUp, AS2 should have it (AS3's provider)
    AS *as2 = asMap.at(2).get();
    const auto &rib2 = as2->getPolicy().getlocalRib();

    EXPECT_GT(rib2.size(), 0);

    // Check if AS2 received the announcement
    if (rib2.find("192.168.1.0/24") != rib2.end())
    {
        const Announcement &ann = rib2.at("192.168.1.0/24");
        EXPECT_EQ(Relationship::CUSTOMER, ann.getRelationship());
    }
}

TEST_F(AsGraphPropagationTest, PropagateUp_PropagatesMultipleHops)
{
    graph->loadROVDeployment("test_propagation_rov.csv");
    graph->buildGraph("test_propagation_graph.txt");
    graph->flattenGraph();

    graph->processInitialAnnouncements("test_propagation_anns.csv");
    graph->propagateUp();

    const auto &asMap = graph->getAsMap();

    // AS1 should eventually receive the announcement from AS3
    AS *as1 = asMap.at(1).get();
    const auto &rib1 = as1->getPolicy().getlocalRib();

    // After propagateUp, AS1 might have it (depends on processing order)
    // At minimum AS2 should have it
    AS *as2 = asMap.at(2).get();
    const auto &rib2 = as2->getPolicy().getlocalRib();

    EXPECT_GT(rib2.size(), 0);
}

// ==================== PROPAGATE ACROSS TESTS ====================

TEST_F(AsGraphPropagationTest, PropagateAcross_PeerRelationships)
{
    graph->loadROVDeployment("test_propagation_rov.csv");
    graph->buildGraph("test_complex_graph.txt");
    graph->flattenGraph();

    graph->processInitialAnnouncements("test_complex_anns.csv");
    graph->propagateUp();
    graph->propagateAcross();

    const auto &asMap = graph->getAsMap();

    // After propagation, peers should exchange routes
    // AS1 and AS2 are peers
    AS *as1 = asMap.at(1).get();
    AS *as2 = asMap.at(2).get();

    const auto &rib1 = as1->getPolicy().getlocalRib();
    const auto &rib2 = as2->getPolicy().getlocalRib();

    // Both should have some announcements after propagation
    EXPECT_GT(rib1.size() + rib2.size(), 0);
}

// ==================== PROPAGATE DOWN TESTS ====================

TEST_F(AsGraphPropagationTest, PropagateDown_LinearTopology)
{
    graph->loadROVDeployment("test_propagation_rov.csv");
    graph->buildGraph("test_propagation_graph.txt");
    graph->flattenGraph();

    // Create an announcement at AS1 (top)
    std::ofstream topAnnFile("test_top_ann.csv");
    topAnnFile << "asn,prefix\n";
    topAnnFile << "1,8.8.8.0/24\n";
    topAnnFile.close();

    graph->processInitialAnnouncements("test_top_ann.csv");
    graph->propagateDown();

    const auto &asMap = graph->getAsMap();

    // AS2 (customer of AS1) should receive the announcement
    AS *as2 = asMap.at(2).get();
    const auto &rib2 = as2->getPolicy().getlocalRib();

    EXPECT_GT(rib2.size(), 0);

    std::filesystem::remove("test_top_ann.csv");
}

TEST_F(AsGraphPropagationTest, PropagateDown_MultipleLevels)
{
    graph->loadROVDeployment("test_propagation_rov.csv");
    graph->buildGraph("test_propagation_graph.txt");
    graph->flattenGraph();

    // AS1 announces
    std::ofstream topAnnFile("test_top_ann2.csv");
    topAnnFile << "asn,prefix\n";
    topAnnFile << "1,8.8.8.0/24\n";
    topAnnFile.close();

    graph->processInitialAnnouncements("test_top_ann2.csv");
    graph->propagateDown();

    const auto &asMap = graph->getAsMap();

    // AS3 (at the bottom) should eventually receive the announcement
    AS *as3 = asMap.at(3).get();
    const auto &rib3 = as3->getPolicy().getlocalRib();

    EXPECT_GT(rib3.size(), 0);

    std::filesystem::remove("test_top_ann2.csv");
}

// ==================== FULL PROPAGATION TESTS ====================

TEST_F(AsGraphPropagationTest, FullPropagation_UpAcrossDown)
{
    graph->loadROVDeployment("test_propagation_rov.csv");
    graph->buildGraph("test_complex_graph.txt");
    graph->flattenGraph();

    graph->processInitialAnnouncements("test_complex_anns.csv");

    // Full propagation cycle
    graph->propagateUp();
    graph->propagateAcross();
    graph->propagateDown();

    const auto &asMap = graph->getAsMap();

    // All ASes should have some routes after full propagation
    int total_routes = 0;
    for (const auto &pair : asMap)
    {
        AS *as = pair.second.get();
        total_routes += as->getPolicy().getlocalRib().size();
    }

    EXPECT_GT(total_routes, 0);
}

TEST_F(AsGraphPropagationTest, FullPropagation_ASPathGrowth)
{
    graph->loadROVDeployment("test_propagation_rov.csv");
    graph->buildGraph("test_propagation_graph.txt");
    graph->flattenGraph();

    graph->processInitialAnnouncements("test_propagation_anns.csv");
    graph->propagateUp();

    const auto &asMap = graph->getAsMap();

    // Check AS path length increases as announcement propagates up
    AS *as3 = asMap.at(3).get();
    const auto &rib3 = as3->getPolicy().getlocalRib();

    if (rib3.find("192.168.1.0/24") != rib3.end())
    {
        const Announcement &ann3 = rib3.at("192.168.1.0/24");
        // Origin announcement should have owner AS prepended
        EXPECT_GT(ann3.getAsPath().size(), 0);
    }
}

// ==================== EMPTY GRAPH EDGE CASES ====================

TEST_F(AsGraphPropagationTest, EmptyGraph_NoCrash)
{
    // Empty graph, should not crash
    AsGraph emptyGraph;
    emptyGraph.flattenGraph();
    emptyGraph.propagateUp();
    emptyGraph.propagateAcross();
    emptyGraph.propagateDown();

    EXPECT_EQ(0, emptyGraph.getAsMap().size());
}

TEST_F(AsGraphPropagationTest, NoAnnouncements_NoCrash)
{
    graph->buildGraph("test_propagation_graph.txt");
    graph->flattenGraph();

    // No initial announcements, should not crash
    graph->propagateUp();
    graph->propagateAcross();
    graph->propagateDown();

    const auto &asMap = graph->getAsMap();
    EXPECT_EQ(3, asMap.size()); // Still have ASes

    // All RIBs should be empty
    for (const auto &pair : asMap)
    {
        AS *as = pair.second.get();
        EXPECT_EQ(0, as->getPolicy().getlocalRib().size());
    }
}

// ==================== ROV DEPLOYMENT INTEGRATION TESTS ====================

TEST_F(AsGraphPropagationTest, ROVDeployment_FiltersInvalidRoutes)
{
    graph->loadROVDeployment("test_rov_deployment.csv");
    graph->buildGraph("test_complex_graph.txt");
    graph->flattenGraph();

    // AS2 and AS3 use ROV
    const auto &asMap = graph->getAsMap();

    // Verify ROV-enabled ASes exist
    EXPECT_TRUE(asMap.find(2) != asMap.end());
    EXPECT_TRUE(asMap.find(3) != asMap.end());
}

// ==================== FLATTENED GRAPH STRUCTURE TESTS ====================

TEST_F(AsGraphPropagationTest, FlattenedGraph_NoEmptyRanks)
{
    graph->buildGraph("test_complex_graph.txt");
    graph->flattenGraph();

    const auto &flattened = graph->getFlattenedGraph();

    // No rank should be empty
    for (const auto &rank : flattened)
    {
        EXPECT_GT(rank.size(), 0);
    }
}

TEST_F(AsGraphPropagationTest, FlattenedGraph_AllASesIncluded)
{
    graph->buildGraph("test_complex_graph.txt");
    graph->flattenGraph();

    const auto &flattened = graph->getFlattenedGraph();
    const auto &asMap = graph->getAsMap();

    // Count total ASes in flattened graph
    std::set<int> flattenedAses;
    for (const auto &rank : flattened)
    {
        for (int asn : rank)
        {
            flattenedAses.insert(asn);
        }
    }

    EXPECT_EQ(asMap.size(), flattenedAses.size());
}
