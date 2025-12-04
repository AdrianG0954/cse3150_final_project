#include <iostream>
#include <fstream>
#include <string>
#include <memory>
#include <thread>

#include "AsGraph.h"
#include "Utils.h"
#include "Relationships.h"

using std::cout, std::endl, std::cerr,
    std::string, std::vector, std::unique_ptr,
    std::ifstream, std::unordered_set,
    std::make_unique, std::thread;

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

bool AsGraph::nodeHasCycle(int src)
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
        RelationshipType relType = static_cast<RelationshipType>(stoi(tokens[2]));

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

        if (relType == RelationshipType::PEER_TO_PEER)
        {
            // 0 = peer-to-peer (bidirectional)
            adjacencyList[dstAsn].emplace_back(srcAsn, relType);
            asMap[srcAsn]->addPeer(dstAsn);
            asMap[dstAsn]->addPeer(srcAsn);
        }
        else if (relType == RelationshipType::PROVIDER_TO_CUSTOMER)
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
    unordered_map<int, int> rankMap;
    vector<int> ranks;
    for (const auto &pair : asMap)
    {
        int asId = pair.first;
        AS *as = pair.second.get();
        if (as->getCustomers().empty())
        {
            rankMap[asId] = 0;
            ranks.push_back(asId);
        }
        else
        {
            rankMap[asId] = -1;
        }
    }

    int rankSize = 1;
    for (size_t i = 0; i < ranks.size(); ++i)
    {
        int asn = ranks[i];
        AS *as = asMap[asn].get();

        int newRank = rankMap[asn] + 1;
        rankSize = std::max(rankSize, newRank);
        for (int pAsn : as->getProviders())
        {
            if (newRank > rankMap[pAsn])
            {
                rankMap[pAsn] = newRank;
                ranks.push_back(pAsn);
            }
        }
    }

    // create flattened graph (allocate space for easy insertion)
    flattenedGraph.resize(rankSize);
    for (const auto &pair : rankMap)
    {
        int asn = pair.first;
        int rank = pair.second;
        flattenedGraph[rank].push_back(asn);
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

        int asn = stoi(res[0]);
        string prefix = res[1];
        string rovStr = res[2];
        if (!rovStr.empty() && rovStr.back() == '\r')
        {
            rovStr.pop_back();
        }
        bool rovInvalid = (rovStr == "True") ? true : false;

        if (asMap.find(asn) == asMap.end())
        {
            cerr << "ASN: " << asn << " not found." << endl;
            continue;
        }
        AS *as = asMap[asn].get();
        Announcement a(prefix, {asn}, asn, Relationship::ORIGIN, rovInvalid);

        Policy &policy = as->getPolicy();
        policy.addOrigin(a);
    }
    file.close();
}

void processAnnouncementRange(const vector<int> &asns, size_t start, size_t end,
                              unordered_map<int, unique_ptr<AS>> &asMap)
{
    /*
    This is the helper function each thread uses to process their half of the work.

    It takes as arguments the list of rank ASNs, the start and end indices for their work
    and and asMap for accessing the AS references.
     */
    for (size_t i = start; i < end; ++i)
    {
        int asn = asns[i];
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
            AS *as = asMap[cAsn].get();
            Policy &policy = as->getPolicy();
            const auto &rib = policy.getlocalRib();

            // send original nodes announcements to providers
            for (int pAsn : as->getProviders())
            {
                AS *providerAs = asMap[pAsn].get();

                for (const auto &ann : rib)
                {
                    const Announcement &currAnn = ann.second;

                    // Reuse announcement object, only change relationship and nextHop
                    providerAs->getPolicy().enqueueAnnouncement(
                        Announcement(currAnn.getPrefix(), currAnn.getAsPath(),
                                     cAsn, Relationship::CUSTOMER, currAnn.isRovInvalid()));
                }
            }
        }

        // before moving, process next rank's announcements with 2 threads
        if (currRank + 1 < flattenedGraph.size())
        {
            const vector<int> &nextRank = flattenedGraph[currRank + 1];
            size_t midpoint = nextRank.size() / 2;

            thread t1(processAnnouncementRange, std::cref(nextRank), 0, midpoint, std::ref(asMap));
            thread t2(processAnnouncementRange, std::cref(nextRank), midpoint, nextRank.size(), std::ref(asMap));

            t1.join();
            t2.join();
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

            for (const auto &p : rib)
            {
                const Announcement &currAnn = p.second;

                peerAs->getPolicy().enqueueAnnouncement(
                    Announcement(currAnn.getPrefix(), currAnn.getAsPath(),
                                 as->getAsn(), Relationship::PEER, currAnn.isRovInvalid()));
            }
        }
    }

    // after all enqueuing, process all announcements with 2 threads
    vector<int> allAsns;
    allAsns.reserve(asMap.size());
    for (const auto &pair : asMap)
    {
        allAsns.push_back(pair.first);
    }

    size_t midpoint = allAsns.size() / 2;
    thread t1(processAnnouncementRange, std::cref(allAsns), 0, midpoint, std::ref(asMap));
    thread t2(processAnnouncementRange, std::cref(allAsns), midpoint, allAsns.size(), std::ref(asMap));

    t1.join();
    t2.join();
}

void AsGraph::propagateDown()
{
    /*
    for all ASNs in rank i

    - iterate through their customers (if any)
    - send announcements to their customers

    we do this for each rank iteratively, going from top to bottom
    */
    for (int currRank = flattenedGraph.size() - 1; currRank >= 0; --currRank)
    {

        // process announcements for current rank first
        const vector<int> &currRankAsns = flattenedGraph[currRank];
        size_t midpoint = currRankAsns.size() / 2;

        thread t1(processAnnouncementRange, std::cref(currRankAsns), 0, midpoint, std::ref(asMap));
        thread t2(processAnnouncementRange, std::cref(currRankAsns), midpoint, currRankAsns.size(), std::ref(asMap));

        t1.join();
        t2.join();

        // Then, send announcements from current rank to their customers
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

                for (const auto &p : rib)
                {
                    const Announcement &currAnn = p.second;

                    customerAs->getPolicy().enqueueAnnouncement(
                        Announcement(currAnn.getPrefix(), currAnn.getAsPath(),
                                     asn, Relationship::PROVIDER, currAnn.isRovInvalid()));
                }
            }
        }
    }
}
