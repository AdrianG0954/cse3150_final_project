#include <iostream>
#include <string>
#include <memory>
#include <unordered_map>
#include <vector>
#include <chrono>
#include <sstream>
#include <cstdlib>

#include "AS.h"
#include "AsGraph.h"

using std::cout, std::endl, std::string,
    std::vector, std::unordered_map, std::ostringstream,
    std::ofstream, std::cerr;

string pathPrefix = "/Users/adriangarcia/Desktop/cse_uconn/cse_3150_submissions/project/";

/*
List of what needs to be done for main file:
- Build AS graph from input file (using bench directory)
    - Load ROV deployment file (rov_deployment.txt) and determine what ASes use ROV
    - Display basic info about the graph (number of ASes, relationships)
    - Check for cycles in provider-to-customer relationships
- Process initial announcements
- Propagate announcements up, across, and down the graph
- Compare output with ribs in bench directory
*/

using std::cout, std::endl;

int main()
{
    auto overallStart = std::chrono::high_resolution_clock::now();
    // building the graph
    AsGraph graph;

    int err = graph.loadROVDeployment(pathPrefix + "bench/many/rov_asns.csv");
    if (err != 0)
    {
        cout << "Error loading ROV deployment file." << endl;
        return -1;
    }

    err = graph.buildGraph(pathPrefix + "bench/many/CAIDAASGraphCollector_2025.10.15.txt");
    if (err != 0)
    {
        cout << "Error building AS graph." << endl;
        return -1;
    }
    graph.flattenGraph();

    bool hasCycle = graph.hasCycle();
    if (hasCycle)
    {
        cerr << "Cannot have cycle!!!\n Something with code is incorrect." << endl;
        return -1;
    }

    graph.processInitialAnnouncements(pathPrefix + "bench/many/anns.csv");

    graph.propagateUp();
    graph.propagateAcross();
    graph.propagateDown();

    auto overallEnd = std::chrono::high_resolution_clock::now();
    auto overallElapsed = std::chrono::duration_cast<std::chrono::milliseconds>(overallEnd - overallStart);
    cout << "\nOverall Time elapsed: " << overallElapsed.count() << " ms\n"
         << endl;

    // outputting info to my_output.csv
    cout << "Outputting RIBs to outputs/my_output.csv..." << endl;

    std::ofstream outfile(pathPrefix + "cse3150_final_project/output/my_output.csv");
    if (!outfile.is_open())
    {
        cerr << "Failed to open output file!" << endl;
        return -1;
    }

    outfile << "asn,prefix,as_path" << std::endl;
    const auto &asMap = graph.getAsMap();
    for (const auto &pair : asMap)
    {
        int asn = pair.first;
        AS *as = pair.second.get();
        const auto &localRib = as->getPolicy().getlocalRib();
        for (const auto &entry : localRib)
        {
            const string &prefix = entry.first;
            const Announcement &ann = entry.second;
            const vector<int> &currPath = ann.getAsPath();

            ostringstream asPath;
            asPath << "\"(";
            for (size_t i = 0; i < currPath.size(); ++i)
            {
                asPath << currPath[i];
                if (i < currPath.size() - 1)
                {
                    asPath << ", ";
                }
            }
            if (currPath.size() == 1)
            {
                asPath << ",";
            }
            asPath << ")\"";
            outfile << asn << "," << prefix << "," << asPath.str() << endl;
        }
    }
    outfile.close();

    // run the comparison script
    cout << "Comparing output with expected RIBs..." << endl;
    string command = "bash " + pathPrefix + "bench/compare_output.sh " + pathPrefix + "bench/many/ribs.csv " + pathPrefix + "cse3150_final_project/output/my_output.csv";
    int result = std::system(command.c_str());
    if (result == 0)
    {
        cout << "\nAll tests passed! Output matches expected RIBs." << endl;
    }
    else
    {
        cerr << "\nTests failed. Check the diff output above." << endl;
        return -1;
    }
    return 0;
}
