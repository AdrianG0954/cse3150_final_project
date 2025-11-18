#pragma once

#include <iostream>
#include <fstream>
#include <unordered_map>
#include <memory>
#include <vector>
#include "AS.h"
#include <string>
#include <unordered_set>

class AsGraph
{
private:
    std::unordered_map<int, std::unique_ptr<AS>> asMap;
    std::unordered_map<int, std::vector<std::pair<int, int>>> adjacencyList;

    bool hasCycle_helper(int src, int relationship, std::unordered_set<int> &visited, std::unordered_set<int> &safe)
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
            if (neiRelationship == relationship && hasCycle_helper(neiNode, relationship, visited, safe))
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

    std::vector<std::string> split(const std::string &s, const char delimeter);

    void buildGraph(const std::string &fileName);

    // must return ref to avoid copying the map
    const auto &getAsMap() const
    {
        return asMap;
    }

    const auto &getAdjacencyList() const
    {
        return adjacencyList;
    }

    bool hasCycle(int relationship);

    bool NodeHasCycle(int src, int relationship);
};
