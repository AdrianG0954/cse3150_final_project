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

void AsGraph::loadROVDeployment(const std::string &filename)
{
    std::ifstream file(filename);
    if (!file.is_open())
    {
        std::cerr << "ROV deployment file not found: " << filename << std::endl;
        return;
    }

    std::string line;
    while (std::getline(file, line))
    {
        if (!line.empty())
        {
            int asn = std::stoi(line);
            rovEnabledAsns.insert(asn);
        }
    }
    file.close();
    
    std::cout << "Loaded " << rovEnabledAsns.size() << " ROV-enabled ASes" << std::endl;
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
        std::vector<std::string> tokens = Utils::split(line, '|');

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
            bool useROV = (rovEnabledAsns.find(srcAsn) != rovEnabledAsns.end());
            asMap[srcAsn] = std::make_unique<AS>(srcAsn, useROV);
        }
        if (asMap.find(dstAsn) == asMap.end())
        {
            bool useROV = (rovEnabledAsns.find(dstAsn) != rovEnabledAsns.end());
            asMap[dstAsn] = std::make_unique<AS>(dstAsn, useROV);
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

void AsGraph::processInitialAnnouncements(const std::string &filename)
{
    std::ifstream file(filename);
    if (!file.is_open())
    {
        std::cerr << "File failed to open" << std::endl;
        return; // nothing to do if seeds file can't be read
    }

    std::string line;
    vector<int> seededAsns;
    while (getline(file, line))
    {
        vector<string> res = Utils::split(line, ',');

        int asn = std::stoi(res[0]); // seed_asn
        string prefix = res[1];      // nodes prefix
        bool rovInvalid = (res[2] == "True") ? true : false;

        // enqueue the announcement
        if (asMap.find(asn) == asMap.end())
        {
            cerr << "ASN: " << asn << " not found." << endl;
        }
        seededAsns.push_back(asn);
        AS *as = asMap[asn].get();
        Announcement a(prefix, {asn}, asn, "origin", rovInvalid);

        // enqueue announcement to AS policy
        Policy &policy = as->getPolicy();
        policy.enqueueAnnouncement(a);
    }

    for (int asn : seededAsns)
    {
        AS *as = asMap[asn].get();
        as->getPolicy().processAnnouncements();
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
            Policy &policy = as->getPolicy();
            const auto &rib = policy.getlocalRib();

            // send original nodes announcements to providers
            for (int pAsn : as->getProviders())
            {
                AS *providerAs = asMap[pAsn].get();

                // going through original nodes announcements
                for (const auto &p : rib)
                {
                    const Announcement &currAnn = p.second;

                    Announcement toSend(
                        currAnn.getPrefix(),
                        currAnn.getAsPath(),
                        cAsn,
                        "customer");

                    providerAs->getPolicy().enqueueAnnouncement(toSend);
                }
            }
        }

        // before moving, process next rank's announcements
        if (currRank + 1 < flattenedGraph.size())
        {
            for (int asn : flattenedGraph[currRank + 1])
            {
                AS *as = asMap[asn].get();
                as->getPolicy().processAnnouncements();
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
        Policy &policy = as->getPolicy();
        const auto &rib = policy.getlocalRib();
        for (int peerAsn : as->getPeers())
        {
            AS *peerAs = asMap[peerAsn].get();

            // going through original nodes announcements
            for (const auto &p : rib)
            {
                const Announcement &currAnn = p.second;

                Announcement toSend(
                    currAnn.getPrefix(),
                    currAnn.getAsPath(),
                    as->getAsn(),
                    "peer");

                peerAs->getPolicy().enqueueAnnouncement(toSend);
            }
        }
    }

    // after all enqueuing, process all announcements
    for (const auto &pair : asMap)
    {
        AS *as = pair.second.get();
        as->getPolicy().processAnnouncements();
    }
}

void AsGraph::propagateDown()
{
    /*
    for all ASNs in rank n

    - iterate through their customers (if any)
    - send announcements to their providers

    we do this for each rank iteratively
    */
    for (int currRank = flattenedGraph.size() - 1; currRank >= 0; --currRank)
    {
        for (int asn : flattenedGraph[currRank])
        {
            // gather information from original node
            AS *as = asMap[asn].get();
            Policy &policy = as->getPolicy();
            const auto &rib = policy.getlocalRib();

            // send original nodes announcements to customers
            for (int cAsn : as->getCustomers())
            {
                AS *customerAs = asMap[cAsn].get();

                // going through original nodes announcements
                for (const auto &p : rib)
                {
                    const Announcement &currAnn = p.second;

                    Announcement toSend(
                        currAnn.getPrefix(),
                        currAnn.getAsPath(),
                        asn,
                        "provider");

                    customerAs->getPolicy().enqueueAnnouncement(toSend);
                }
            }
        }

        // before moving, process next rank's announcements
        if (currRank - 1 >= 0)
        {
            for (int asn : flattenedGraph[currRank - 1])
            {
                AS *as = asMap[asn].get();
                as->getPolicy().processAnnouncements();
            }
        }
    }
}
