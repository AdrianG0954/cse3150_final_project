#include <iostream>
#include "AsGraph.h"

using std::cout;

int main()
{
    AsGraph graph;

    // should result in 3 AS nodes
    graph.buildGraph("test.txt");

    const auto &asMap = graph.getAsMap();
    cout << "Total AS nodes: " << asMap.size() << std::endl;
    for (const auto &pair : asMap)
    {
        cout << "AS ID: " << pair.first << std::endl;

        // Display all relationships
        const AS *asNode = pair.second.get();
        cout << "  Providers: ";
        for (int provider : asNode->getProviders())
        {
            cout << provider << " ";
        }
        cout << std::endl;

        cout << "  Customers: ";
        for (int customer : asNode->getCustomers())
        {
            cout << customer << " ";
        }
        cout << std::endl;

        cout << "  Peers: ";
        for (int peer : asNode->getPeers())
        {
            cout << peer << " ";
        }
        cout << std::endl;
    }

    // Check for cycles in provider-to-customer relationships (1)
    bool hasCycle = graph.hasCycle();
    cout << "\nGraph has provider-to-customer cycle: " << (hasCycle ? "Yes" : "No") << std::endl;

    return 0;
}
