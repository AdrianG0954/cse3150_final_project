#include <gtest/gtest.h>
#include "AsGraph.h"
#include <fstream>
#include <sstream>
#include <filesystem>
#include <algorithm>

class AsGraphTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        graph = new AsGraph();
        createTestDataFiles();
    }

    void TearDown() override
    {
        delete graph;
        // Clean up test files
        std::filesystem::remove("test_simple.txt");
        std::filesystem::remove("test_cycle.txt");
        std::filesystem::remove("test_peers.txt");
        std::filesystem::remove("test_empty.txt");
    }

    void createTestDataFiles()
    {
        // Simple test data
        std::ofstream simple("test_simple.txt");
        simple << "1|2|0|bgp\n";  // AS1 and AS2 are peers
        simple << "2|3|1|bgp\n";  // AS2 is provider to AS3
        simple << "3|4|-1|bgp\n"; // AS3 is customer of AS4
        simple.close();

        // Test data with cycle
        std::ofstream cycle("test_cycle.txt");
        cycle << "1|2|1|bgp\n"; // AS1 provider to AS2
        cycle << "2|3|1|bgp\n"; // AS2 provider to AS3
        cycle << "3|1|1|bgp\n"; // AS3 provider to AS1 (creates cycle)
        cycle.close();

        // Test data with peers
        std::ofstream peers("test_peers.txt");
        peers << "100|200|0|bgp\n";  // Peers
        peers << "200|300|0|bgp\n";  // Peers
        peers << "300|400|1|bgp\n";  // Provider-customer
        peers << "400|500|-1|bgp\n"; // Customer-provider
        peers.close();

        // Empty test file
        std::ofstream empty("test_empty.txt");
        empty.close();
    }

    // Helper function to check if a vector contains a specific value
    template <typename T>
    bool contains(const std::vector<T> &vec, const T &value)
    {
        return std::find(vec.begin(), vec.end(), value) != vec.end();
    }

    AsGraph *graph;
};

// Test split function
TEST_F(AsGraphTest, SplitFunction)
{
    std::vector<std::string> result = graph->split("a|b|c", '|');
    EXPECT_EQ(3, result.size());
    EXPECT_EQ("a", result[0]);
    EXPECT_EQ("b", result[1]);
    EXPECT_EQ("c", result[2]);

    result = graph->split("", '|');
    EXPECT_EQ(0, result.size());

    result = graph->split("single", '|');
    EXPECT_EQ(1, result.size());
    EXPECT_EQ("single", result[0]);

    result = graph->split("a|b|", '|');
    EXPECT_EQ(2, result.size());
    EXPECT_EQ("a", result[0]);
    EXPECT_EQ("b", result[1]);
}

// Test basic graph building
TEST_F(AsGraphTest, BuildGraphBasic)
{
    graph->buildGraph("test_simple.txt");

    const auto &asMap = graph->getAsMap();
    const auto &adjList = graph->getAdjacencyList();

    EXPECT_EQ(4, asMap.size());

    // Check that all expected AS numbers exist
    EXPECT_TRUE(asMap.find(1) != asMap.end());
    EXPECT_TRUE(asMap.find(2) != asMap.end());
    EXPECT_TRUE(asMap.find(3) != asMap.end());
    EXPECT_TRUE(asMap.find(4) != asMap.end());

    // Check adjacency list structure
    EXPECT_TRUE(adjList.find(1) != adjList.end());
    EXPECT_TRUE(adjList.find(2) != adjList.end());
}

// Test AS relationships after graph building
TEST_F(AsGraphTest, ASRelationships)
{
    graph->buildGraph("test_simple.txt");

    const auto &asMap = graph->getAsMap();

    // Ensure nodes have correct relationships
    const AS *as1 = asMap.at(1).get();
    EXPECT_EQ(1, as1->getPeers().size());
    EXPECT_EQ(0, as1->getProviders().size());
    EXPECT_EQ(0, as1->getCustomers().size());
    EXPECT_TRUE(contains(as1->getPeers(), 2));

    const AS *as2 = asMap.at(2).get();
    EXPECT_EQ(1, as2->getPeers().size());
    EXPECT_EQ(0, as2->getProviders().size());
    EXPECT_EQ(1, as2->getCustomers().size());
    EXPECT_TRUE(contains(as2->getPeers(), 1));
    EXPECT_TRUE(contains(as2->getCustomers(), 3));

    const AS *as3 = asMap.at(3).get();
    EXPECT_EQ(0, as3->getPeers().size());
    EXPECT_EQ(2, as3->getProviders().size()); 
    EXPECT_EQ(0, as3->getCustomers().size());
    EXPECT_TRUE(contains(as3->getProviders(), 2));
    EXPECT_TRUE(contains(as3->getProviders(), 4));

    const AS *as4 = asMap.at(4).get();
    EXPECT_EQ(0, as4->getPeers().size());
    EXPECT_EQ(0, as4->getProviders().size());
    EXPECT_EQ(1, as4->getCustomers().size());
    EXPECT_TRUE(contains(as4->getCustomers(), 3));
}

// Test bidirectional peer relationships
TEST_F(AsGraphTest, PeerRelationships)
{
    graph->buildGraph("test_peers.txt");

    const auto &asMap = graph->getAsMap();
    const auto &adjList = graph->getAdjacencyList();

    // Check peer relationship between AS100 and AS200
    const auto &as100_neighbors = adjList.at(100);
    const auto &as200_neighbors = adjList.at(200);

    bool as100_has_as200_peer = false;
    bool as200_has_as100_peer = false;

    for (const auto &neighbor : as100_neighbors)
    {
        if (neighbor.first == 200 && neighbor.second == 0)
        {
            as100_has_as200_peer = true;
            break;
        }
    }

    for (const auto &neighbor : as200_neighbors)
    {
        if (neighbor.first == 100 && neighbor.second == 0)
        {
            as200_has_as100_peer = true;
            break;
        }
    }

    EXPECT_TRUE(as100_has_as200_peer);
    EXPECT_TRUE(as200_has_as100_peer);

    // Check AS object peer lists
    const AS *as100 = asMap.at(100).get();
    const AS *as200 = asMap.at(200).get();
    EXPECT_TRUE(contains(as100->getPeers(), 200));
    EXPECT_TRUE(contains(as200->getPeers(), 100));
}

// Test cycle detection
TEST_F(AsGraphTest, CycleDetection)
{
    graph->buildGraph("test_simple.txt");
    EXPECT_FALSE(graph->hasCycle()); 

    AsGraph cycleGraph;
    cycleGraph.buildGraph("test_cycle.txt");
    
    EXPECT_TRUE(cycleGraph.NodeHasCycle(1));
    EXPECT_TRUE(cycleGraph.NodeHasCycle(2));
    EXPECT_TRUE(cycleGraph.NodeHasCycle(3));
}

// Test empty graph
TEST_F(AsGraphTest, EmptyGraph)
{
    graph->buildGraph("test_empty.txt");

    const auto &asMap = graph->getAsMap();
    const auto &adjList = graph->getAdjacencyList();

    EXPECT_EQ(0, asMap.size());
    EXPECT_EQ(0, adjList.size());
    EXPECT_FALSE(graph->hasCycle());
}

// Test complex graph with multiple relationship types
TEST_F(AsGraphTest, ComplexGraph)
{
    graph->buildGraph("test_peers.txt");

    const auto &asMap = graph->getAsMap();
    EXPECT_EQ(5, asMap.size());

    const AS *as100 = asMap.at(100).get();
    const AS *as200 = asMap.at(200).get();
    const AS *as300 = asMap.at(300).get();

    EXPECT_TRUE(contains(as100->getPeers(), 200));
    EXPECT_TRUE(contains(as200->getPeers(), 100));
    EXPECT_TRUE(contains(as200->getPeers(), 300));
    EXPECT_TRUE(contains(as300->getPeers(), 200));

    // Test provider-customer relationships
    EXPECT_TRUE(contains(as300->getCustomers(), 400));

    const AS *as400 = asMap.at(400).get();
    EXPECT_TRUE(contains(as400->getProviders(), 300));
    EXPECT_TRUE(contains(as400->getProviders(), 500));

    const AS *as500 = asMap.at(500).get();
    EXPECT_TRUE(contains(as500->getCustomers(), 400));
}

// Test nonexistent file handling
TEST_F(AsGraphTest, NonexistentFile)
{
    // This should not crash but handle the error gracefully
    graph->buildGraph("nonexistent_file.txt");

    const auto &asMap = graph->getAsMap();
    EXPECT_EQ(0, asMap.size());
}
