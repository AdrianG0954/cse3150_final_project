#include <iostream>
#include "AsGraph.h"
#include <fstream>
#include <string>
#include <memory>

std::vector<std::string> AsGraph::split(const std::string &s, const char delimeter)
{
    std::vector<std::string> res;
    size_t start = 0, posEnd = 0;
    while ((posEnd = s.find(delimeter, start)) != std::string::npos)
    {
        res.push_back(s.substr(start, posEnd - start));
        start = posEnd + 1;
    }
    if (start < s.size())
        res.push_back(s.substr(start));

    return res;
}

void AsGraph::buildGraph(const std::string &fileName)
{
    std::ifstream input;
    input.open(fileName);

    if (input.fail())
    {
        std::cerr << "Error opening the file " << fileName << std::endl;
        return;
    }

    /*
        0 - peer-to-peer
        1 - provider-to-customer
        -1 - customer-to-provider

        1|11537|0|bgp = AS1 and AS11537 are peers
        1|21616|-1|bgp = AS1 is a customer of AS21616
        1|34732|-1|bgp = AS1 is a customer of AS34732
    */
    std::string line;
    while (std::getline(input, line))
    {
        if (line.find("source") != std::string::npos)
        {
            continue;
        }
        std::vector<std::string> tokens = split(line, '|');

        /*
        ex line: 51823|198047|0|mlp:

                51823 - src
                198047 - dst
                0 - relationship type
                mlp: - ignore
        */
        int srcAsn = std::stoi(tokens[0]);
        int dstAsn = std::stoi(tokens[1]);
        int relType = std::stoi(tokens[2]);

        // Create AS nodes for future quick access
        if (asMap.find(srcAsn) == asMap.end())
        {
            asMap[srcAsn] = std::make_unique<AS>(srcAsn);
        }
        if (asMap.find(dstAsn) == asMap.end())
        {   
            asMap[dstAsn] = std::make_unique<AS>(dstAsn);
        }

        // we use emplace to directly create the pair in the vector
        adjacencyList[srcAsn].emplace_back(dstAsn, relType);

        // keeping track of each nodes relationships
        if (relType == 0)
        {
            asMap[srcAsn]->addPeer(dstAsn);
        }
        else if (relType == 1)
        {
            asMap[srcAsn]->addProvider(dstAsn);
        }
        else if (relType == -1)
        {
            asMap[srcAsn]->addCustomer(dstAsn);
        }
    }
}
