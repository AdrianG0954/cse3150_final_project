#include <iostream>
#include "AsGraph.h"
#include <fstream>
#include <string>
#include <memory>
#include "Utils.h"

using std::cout, std::endl, std::cerr,
    std::string, std::vector, std::unique_ptr,
    std::ifstream, std::pair, std::unordered_set,
    std::make_unique;

bool AsGraph::hasCycle()
{
    unordered_set<int> visited;
    unordered_set<int> safe;
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
    unordered_set<int> visited;
    unordered_set<int> safe;
    return hasCycle_helper(src, visited, safe);
}

int AsGraph::loadROVDeployment(const string &filename)
{
    ifstream file(filename);
    if (!file.is_open())
    {
        cerr << "ROV deployment file not found: " << filename << endl;
        return -1;
    }

    string line;
    while (getline(file, line))
    {
        if (!line.empty())
        {
            int asn = stoi(line);
            rovEnabledAsns.insert(asn);
        }
    }
    file.close();

    cout << "Loaded " << rovEnabledAsns.size() << " ROV-enabled ASes" << endl;

    return 0;
}

int AsGraph::buildGraph(const string &fileName)
{
    ifstream input;
    input.open(fileName);

    if (input.fail())
    {
        cerr << "Error opening the file " << fileName << endl;
        return -1;
    }

    string line;
    while (getline(input, line))
    {
        if (line.find("#") != string::npos || line.empty())
        {
            continue;
        }
        vector<string> tokens = Utils::split(line, '|');
        /*
        ex line: 51823|198047|0|mlp:

                51823 - src
                198047 - dst
                0 - relationship type
                mlp: - ignore
        */
        int srcAsn = stoi(tokens[0]);
        int dstAsn = stoi(tokens[1]);
        int relType = stoi(tokens[2]);

        // Create AS nodes for future quick access
        if (asMap.find(srcAsn) == asMap.end())
        {
            bool useROV = (rovEnabledAsns.find(srcAsn) != rovEnabledAsns.end());
            asMap[srcAsn] = make_unique<AS>(srcAsn, useROV);
        }
        if (asMap.find(dstAsn) == asMap.end())
        {
            bool useROV = (rovEnabledAsns.find(dstAsn) != rovEnabledAsns.end());
            asMap[dstAsn] = make_unique<AS>(dstAsn, useROV);
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
        else if (relType == -1)
        {
            // provider-to-customer
            asMap[srcAsn]->addCustomer(dstAsn);
            asMap[dstAsn]->addProvider(srcAsn);
        }
    }
    return 0;
}

void AsGraph::flattenGraph()
{
    vector<int> ex;
    unordered_set<int> alreadyRanked;
    for (const auto &pair : asMap)
    {
        int asId = pair.first;
        AS *as = pair.second.get();
        if (as->getCustomers().empty())
        {
            ex.push_back(asId);
            alreadyRanked.insert(asId);
        }
    }
    flattenedGraph.push_back(ex);

    /*
    we iterate through the prevRanks IDs, and for each ID with providers
    we add them and repeat the process
    */
    int prev = 0;
    while (prev < flattenedGraph.size())
    {
        const vector<int> &prevRank = flattenedGraph[prev];
        unordered_set<int> currRankSet;

        for (int asId : prevRank)
        {
            AS *as = asMap[asId].get();
            for (int pId : as->getProviders())
            {
                if (alreadyRanked.find(pId) != alreadyRanked.end())
                {
                    continue;
                }
                currRankSet.insert(pId);
            }
        }

        if (!currRankSet.empty())
        {
            flattenedGraph.push_back(vector<int>(currRankSet.begin(), currRankSet.end()));

            // mark the ASes as ranked
            for (int asn : currRankSet)
            {
                alreadyRanked.insert(asn);
            }
        }
        prev++;
    }
}

void AsGraph::processInitialAnnouncements(const string &filename)
{
    ifstream file(filename);
    if (!file.is_open())
    {
        cerr << "File failed to open" << endl;
        return;
    }

    string line;
    vector<int> seededAsns;

    // skip header line
    getline(file, line);

    while (getline(file, line))
    {
        vector<string> res = Utils::split(line, ',');

        int asn = stoi(res[0]); // seed_asn
        string prefix = res[1]; // nodes prefix
        bool rovInvalid = (res[2] == "True") ? true : false;

        // enqueue the announcement
        if (asMap.find(asn) == asMap.end())
        {
            cerr << "ASN: " << asn << " not found." << endl;
            continue;
        }
        seededAsns.push_back(asn);
        AS *as = asMap[asn].get();
        Announcement a(prefix, {}, asn, "origin", rovInvalid);

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
                for (const auto &ann : rib)
                {
                    const Announcement &currAnn = ann.second;

                    Announcement toSend(
                        currAnn.getPrefix(),
                        currAnn.getAsPath(),
                        cAsn,
                        "customer",
                        currAnn.isRovInvalid());

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
    */
    for (const auto &pair : asMap)
    {
        AS *as = pair.second.get();
        const Policy &policy = as->getPolicy();
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
                    "peer",
                    currAnn.isRovInvalid());

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
    - send announcements to their customers

    we do this for each rank iteratively, going from top (high rank) to bottom (low rank)
    */
    for (int currRank = flattenedGraph.size() - 1; currRank >= 0; --currRank)
    {
        // First, send announcements from current rank to their customers
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
                        "provider",
                        currAnn.isRovInvalid());

                    customerAs->getPolicy().enqueueAnnouncement(toSend);
                }
            }
        }

        // Then, process the received announcements for the customers (next lower rank)
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
