#pragma once

#include <iostream>
#include <fstream>
#include <unordered_map>
#include <memory>
#include <vector>
#include <string>
#include <unordered_set>

#include "AS.h"

using std::string, std::vector, std::unordered_map, std::pair, std::unique_ptr, std::unordered_set;

class AsGraph
{
private:
    unordered_map<int, unique_ptr<AS>> asMap;                              // mapping of ASN to AS node
    unordered_map<int, vector<pair<int, RelationshipType>>> adjacencyList; // asn -> list of (neighbor_asn, relationship_type)
    vector<vector<int>> flattenedGraph;                                    // ranks of ASNs for propagation
    unordered_set<int> rovEnabledAsns;                                     // ASNs that deploy ROV

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
            RelationshipType neiRelationship = nei.second;
            // we don't follow peer-to-peer edges as they are bidirectional
            if (neiRelationship != RelationshipType::PEER_TO_PEER && hasCycle_helper(neiNode, visited, safe))
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

    /*
    Code that reads caida data and builds the AS graph.
    Returns 0 on success, -1 on failure.
     */
    int buildGraph(const string &fileName);

    const auto &getAsMap() const
    {
        return asMap;
    }

    const auto &getAdjacencyList() const
    {
        return adjacencyList;
    }

    const auto &getFlattenedGraph() const
    {
        return flattenedGraph;
    }

    // check for cycles in the graph (p->c relationships)
    bool hasCycle();

    // check individual nodes for cycles
    bool nodeHasCycle(int src);

    // flatten graph into propagation ranks
    void flattenGraph();

    // stores ASNs that are within rov_asns.csv
    int loadROVDeployment(const string &filename);

    // processes announcements for nodes from anns.csv
    void processInitialAnnouncements(const string &filename);

    // propagates customers announcements to providers
    void propagateUp();

    // propagates peer-to-peer announcements
    void propagateAcross();

    // propagates provider announcements to customers
    void propagateDown();
};
