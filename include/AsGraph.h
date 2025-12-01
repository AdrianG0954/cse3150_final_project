#pragma once

#include <iostream>
#include <fstream>
#include <unordered_map>
#include <memory>
#include <vector>
#include "AS.h"
#include <string>
#include <unordered_set>

using std::string, std::vector, std::unordered_map, std::pair, std::unique_ptr, std::unordered_set;

class AsGraph
{
private:
    unordered_map<int, unique_ptr<AS>> asMap;
    unordered_map<int, vector<pair<int, int>>> adjacencyList;
    vector<vector<int>> flattenedGraph;
    unordered_set<int> rovEnabledAsns; // ASNs that deploy ROV

    bool hasCycle_helper(int src, unordered_set<int> &visited, unordered_set<int> &safe)
    {
        if (safe.find(src) != safe.end())
        {
            return false;
        }
        if (visited.find(src) != visited.end())
        {
            return true;
        }

        visited.insert(src);
        for (const auto &nei : adjacencyList[src])
        {
            int neiNode = nei.first;
            int neiRelationship = nei.second;
            // we don't follow peer-to-peer edges as they are bidirectional
            if (neiRelationship != 0 && hasCycle_helper(neiNode, visited, safe))
            {
                return true;
            }
        }
        visited.erase(src);

        safe.insert(src);
        return false;
    }

public:
    AsGraph() {}

    vector<string> split(const string &s, const char delimiter);

    void buildGraph(const string &fileName);

    // return ref to avoid copying (plus we have to since unique_ptrs)
    const auto &getAsMap() const
    {
        return asMap;
    }

    // return ref to avoid copying
    const auto &getAdjacencyList() const
    {
        return adjacencyList;
    }

    // check for cycles in the graph (p->c relationships)
    bool hasCycle();

    // check an individual node for cycles
    bool NodeHasCycle(int src);

    // flatten graph into propagation ranks
    void flattenGraph();

    const auto &getFlattenedGraph() const
    {
        return flattenedGraph;
    }

    // load ROV deployment file (ASNs that use ROV)
    void loadROVDeployment(const string &filename);

    // process announcements for nodes
    void processInitialAnnouncements(const string &filename);

    void propagateUp();

    void propagateAcross();

    void propagateDown();
};
