#pragma once

#include <iostream>
#include <fstream>
#include <unordered_map>
#include <memory>
#include <vector>
#include "AS.h"
#include <string>

class AsGraph
{
private:
    std::unordered_map<int, std::unique_ptr<AS>> asMap;
    std::unordered_map<int, std::vector<std::pair<int, int>>> adjacencyList;

public:
    AsGraph() {}

    std::vector<std::string> split(const std::string &s, const char delimeter);

    void buildGraph(const std::string &fileName);

    // must return ref to avoid copying the map
    const auto &getAsMap() const
    {
        return asMap;
    }

    const auto getAdjacencyList() const
    {
        return adjacencyList;
    }
};