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
    cout << "Loading rov deployment..."
         << endl;

    int err = graph.loadROVDeployment(pathPrefix + "bench/test/rov_asns.csv");
    cout << "Done! \n"
         << endl;

    if (err != 0)
    {
        cout << "Error loading ROV deployment file." << endl;
        return -1;
    }

    cout << "----------------------------------------" << endl;

    cout << "Building AS graph...\n"
         << endl;

    auto start = std::chrono::high_resolution_clock::now();
    err = graph.buildGraph(pathPrefix + "bench/test/CAIDAASGraphCollector_test.txt");
    if (err != 0)
    {
        cout << "Error building AS graph." << endl;
        return -1;
    }
    auto end = std::chrono::high_resolution_clock::now();

    cout << "Done! \n"
         << endl;

    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    cout << "Time elapsed: " << elapsed.count() << " ms\n"
         << endl;
    cout << "----------------------------------------" << endl;

    cout << "Building ranks for propagation..." << endl;
    start = std::chrono::high_resolution_clock::now();
    graph.flattenGraph();
    end = std::chrono::high_resolution_clock::now();
    cout << "Done! " << endl;

    elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    cout << "Time elapsed: " << elapsed.count() << " ms\n"
         << endl;
    cout << "----------------------------------------" << endl;

    cout << "Graph Information:" << endl;
    const auto &asMap = graph.getAsMap();
    cout << "Total AS nodes: " << asMap.size() << endl;

    // Check for cycles in provider-to-customer relationships (1)
    bool hasCycle = graph.hasCycle();
    cout << "Graph has provider-to-customer cycle: " << (hasCycle ? "Yes" : "No") << endl;
    if (hasCycle)
    {
        cerr << "Cannot have cycle!!!\n Something with code is incorrect." << endl;
        return -1;
    }

    cout << "\nProcessing initial announcements..." << endl;
    graph.processInitialAnnouncements(pathPrefix + "bench/test/anns.csv");
    cout << "Done! " << endl;
    cout << "----------------------------------------" << endl;

    cout << "Propagating announcements through the graph..." << endl;
    graph.propagateUp();
    graph.propagateAcross();
    graph.propagateDown();
    cout << "Done! " << endl;
    cout << "----------------------------------------" << endl;

    // outputting info to my_output.csv
    cout << "Outputting RIBs to outputs/my_output.csv..." << endl;

    std::ofstream outfile(pathPrefix + "cse3150_final_project/output/my_output.csv");
    if (!outfile.is_open())
    {
        cerr << "Failed to open output file!" << endl;
        return -1;
    }

    outfile << "asn,prefix,as_path" << std::endl;
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
    string command = "bash " + pathPrefix + "bench/compare_output.sh " + pathPrefix + "bench/test/ribs.csv " + pathPrefix + "cse3150_final_project/output/my_output.csv";
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
    auto overallEnd = std::chrono::high_resolution_clock::now();
    auto overallElapsed = std::chrono::duration_cast<std::chrono::milliseconds>(overallEnd - overallStart);
    cout << "\nOverall Time elapsed: " << overallElapsed.count() << " ms\n"
         << endl;

    return 0;
}
