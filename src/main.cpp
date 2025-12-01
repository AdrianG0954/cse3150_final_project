#include <iostream>
#include <string>
#include <memory>
#include <unordered_map>
#include <vector>

#include "AS.h"
#include "AsGraph.h"

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
    AsGraph graph;

    // should result in 3 AS nodes
    graph.buildGraph("test.txt");

    const auto &asMap = graph.getAsMap();
    cout << "Total AS nodes: " << asMap.size() << endl;
    for (const auto &pair : asMap)
    {
        cout << "AS ID: " << pair.first << endl;

        // Display all relationships
        const AS *asNode = pair.second.get();
        cout << "  Providers: ";
        for (int provider : asNode->getProviders())
        {
            cout << provider << " ";
        }
        cout << endl;

        cout << "  Customers: ";
        for (int customer : asNode->getCustomers())
        {
            cout << customer << " ";
        }
        cout << endl;

        cout << "  Peers: ";
        for (int peer : asNode->getPeers())
        {
            cout << peer << " ";
        }
        cout << endl;
    }

    // Check for cycles in provider-to-customer relationships (1)
    bool hasCycle = graph.hasCycle();
    cout << "\nGraph has provider-to-customer cycle: " << (hasCycle ? "Yes" : "No") << endl;

    return 0;
}
