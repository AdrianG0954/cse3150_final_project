#include <iostream>
#include <string>
#include <memory>
#include <unordered_map>
#include <vector>
#include <chrono>
#include <sstream>
#include <cstdlib>
#include <filesystem>

#include "AS.h"
#include "AsGraph.h"

namespace fs = std::filesystem;

using std::cout, std::endl, std::string,
    std::vector, std::unordered_map, std::ostringstream,
    std::ofstream, std::cerr;

string pathPrefix = fs::current_path().string() + "/../../";
// current test running
string test = "bench/many";

int main()
{
    auto overallStart = std::chrono::high_resolution_clock::now();

    // building the graph
    AsGraph graph;

    int err = graph.loadROVDeployment(pathPrefix + test + "/rov_asns.csv");
    if (err != 0)
    {
        cout << "Error loading ROV deployment file." << endl;
        return -1;
    }

    err = graph.buildGraph(pathPrefix + test + "/CAIDAASGraphCollector_2025.10.15.txt");
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

    graph.processInitialAnnouncements(pathPrefix + test + "/anns.csv");

    graph.propagateUp();
    graph.propagateAcross();
    graph.propagateDown();

    auto overallEnd = std::chrono::high_resolution_clock::now();
    auto overallElapsed = std::chrono::duration_cast<std::chrono::milliseconds>(overallEnd - overallStart);
    cout << "\nOverall Time elapsed: " << overallElapsed.count() << " ms\n"
         << endl;

    // outputting info to my_output.csv
    cout << "Outputting RIBs to output/my_output.csv..." << endl;

    // Create output directory if it doesn't exist
    std::filesystem::path outputDir = std::filesystem::path(pathPrefix) / "output";
    if (!std::filesystem::exists(outputDir))
    {
        std::filesystem::create_directories(outputDir);
    }

    std::ofstream outfile(pathPrefix + "output/my_output.csv");
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
    string command = "bash " + pathPrefix + "bench/compare_output.sh " + pathPrefix + test + "/ribs.csv " + pathPrefix + "output/my_output.csv";
    int result = std::system(command.c_str());
    if (result == -1)
    {
        cerr << "\nTests failed. Check the diff output above." << endl;
        return -1;
    }
    return 0;
}
