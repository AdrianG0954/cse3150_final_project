#include <iostream>
#include "AsGraph.h"
#include <fstream>
#include <string>
#include <memory>
#include "Utils.h"

using std::cout, std::endl, std::cerr, std::string, std::vector, std::unique_ptr;

bool AsGraph::hasCycle()
{
    std::unordered_set<int> visited;
    std::unordered_set<int> safe;
    for (const auto &pair : adjacencyList)
    {
        int src = pair.first;
        if (hasCycle_helper(src, visited, safe))
        {
            return true;
        }
    }
    return false;
}

bool AsGraph::NodeHasCycle(int src)
{
    std::unordered_set<int> visited;
    std::unordered_set<int> safe;
    return hasCycle_helper(src, visited, safe);
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

        if (relType == 0)
        {
            // bidirectional for peers
            adjacencyList[dstAsn].emplace_back(srcAsn, relType);

            asMap[srcAsn]->addPeer(dstAsn);
            asMap[dstAsn]->addPeer(srcAsn);
        }
        else if (relType == 1)
        {
            // 1 = provider-to-customer
            asMap[srcAsn]->addCustomer(dstAsn);
            asMap[dstAsn]->addProvider(srcAsn);
        }
        else if (relType == -1)
        {
            // -1 = customer-to-provider
            asMap[srcAsn]->addProvider(dstAsn);
            asMap[dstAsn]->addCustomer(srcAsn);
        }
    }
}

void AsGraph::flattenGraph()
{
    std::vector<int> ex;
    for (const auto &pair : asMap)
    {
        int asId = pair.first;
        AS *as = pair.second.get();
        if (as->getCustomers().empty())
        {
            ex.push_back(asId);
        }
    }
    flattenedGraph.push_back(ex);

    /*
    we iterate through the prevRanks IDs, and for each ID with providers
    we add them and repeat the process
    */
    int prev = 0;
    std::vector<int> currRank;
    while (prev < flattenedGraph.size())
    {
        const std::vector<int> &prevRank = flattenedGraph[prev];
        for (int asId : prevRank)
        {
            AS *as = asMap[asId].get();
            for (int pId : as->getProviders())
            {
                currRank.push_back(pId);
            }
        }

        if (!currRank.empty())
        {
            flattenedGraph.push_back(currRank);
            currRank.clear();
        }
        prev++;
    }
}

void AsGraph::processAnnouncements(const std::string &filename)
{
    std::ifstream file(filename);
    if (!file.is_open())
    {
        std::cerr << "File failed to open" << std::endl;
        return; // nothing to do if seeds file can't be read
    }

    std::string line;
    while (getline(file, line))
    {
        vector<string> res = Utils::split(line, ',');

        int asn = std::stoi(res[0]); // seed_asn
        string prefix = res[1];      // nodes prefix
        bool rovInvalid = (res.size() > 2) ? (res[2] == "False") : false;

        // enqueue the announcement
        if (asMap.find(asn) == asMap.end())
        {
            cerr << "ASN: " << asn << " not found." << endl;
        }
        AS *as = asMap[asn].get();
        Announcement a(prefix, {asn}, asn, "origin");

        // enqueue announcement to AS policy
        BGP &bgpPolicy = as->getPolicy();
        bgpPolicy.enqueueAnnouncement(a);
        bgpPolicy.processAnnouncements();
    }
}

void AsGraph::propagateUp()
{
    /*
    for all ASNs in rank 0

    - iterate through their providers (if any)
    - send announcements to their providers

    we do this for each rank iteratively
    */
    for (size_t currRank = 0; currRank < flattenedGraph.size(); ++currRank)
    {
        for (int cAsn : flattenedGraph[currRank])
        {
            // gather information from original node
            AS *as = asMap[cAsn].get();
            BGP &policy = as->getPolicy();
            const auto &rib = policy.getlocalRib();

            // send original nodes announcements to providers
            for (int pAsn : as->getProviders())
            {
                AS *providerAs = asMap[pAsn].get();

                // going through original nodes announcements
                for (const auto &p : rib)
                {
                    const Announcement &currAnn = p.second;

                    // update announcement
                    vector<int> newAsnPath = currAnn.getAsPath();
                    newAsnPath.insert(newAsnPath.begin(), as->getAsn());
                    Announcement toSend(
                        currAnn.getPrefix(),
                        newAsnPath,
                        cAsn,
                        "customer");

                    providerAs->getPolicy().enqueueAnnouncement(toSend);
                }
            }
        }
    }
}

void AsGraph::propagateAcross()
{
    /*
    for all ASNs in our graph, we send their announcements to their peers
    We enqueue all announcements only (no processing)
    */
    for (const auto &pair : asMap)
    {
        AS *as = pair.second.get();
        BGP &policy = as->getPolicy();
        const auto &rib = policy.getlocalRib();
        for (int peerAsn : as->getPeers())
        {
            AS *peerAs = asMap[peerAsn].get();

            // going through original nodes announcements
            for (const auto &p : rib)
            {
                const Announcement &currAnn = p.second;

                // update announcement
                vector<int> newAsnPath = currAnn.getAsPath();
                newAsnPath.insert(newAsnPath.begin(), as->getAsn());
                Announcement toSend(
                    currAnn.getPrefix(),
                    newAsnPath,
                    as->getAsn(),
                    "peer");

                peerAs->getPolicy().enqueueAnnouncement(toSend);
            }
        }
    }
}

void AsGraph::propagateDown()
{
    /*
    for all ASNs in rank 0

    - iterate through their providers (if any)
    - send announcements to their providers

    we do this for each rank iteratively
    */
    for (int currRank = flattenedGraph.size() - 1; currRank >= 0; --currRank)
    {
        for (int asn : flattenedGraph[currRank])
        {
            // gather information from original node
            AS *as = asMap[asn].get();
            BGP &policy = as->getPolicy();
            const auto &rib = policy.getlocalRib();

            // send original nodes announcements to customers
            for (int cAsn : as->getCustomers())
            {
                AS *customerAs = asMap[cAsn].get();

                // going through original nodes announcements
                for (const auto &p : rib)
                {
                    const Announcement &currAnn = p.second;

                    // update announcement
                    vector<int> newAsnPath = currAnn.getAsPath();
                    newAsnPath.insert(newAsnPath.begin(), asn);
                    Announcement toSend(
                        currAnn.getPrefix(),
                        newAsnPath,
                        asn,
                        "provider");

                    customerAs->getPolicy().enqueueAnnouncement(toSend);
                }
            }
        }
    }
}
