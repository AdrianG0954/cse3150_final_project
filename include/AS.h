#pragma once
#include <string>
#include <vector>
#include <memory>

class AS
{
private:
    int asn;
    std::vector<int> providers;
    std::vector<int> customers;
    std::vector<int> peers;

public:
    AS(int asn)
    {
        this->asn = asn;
    }

    int getAsn() const
    {
        return this->asn;
    }

    void addProvider(int providerAsn)
    {
        this->providers.push_back(providerAsn);
    }

    void addCustomer(int customerAsn)
    {
        this->customers.push_back(customerAsn);
    }

    void addPeer(int peerAsn)
    {
        this->peers.push_back(peerAsn);
    }

    const std::vector<int> &getProviders() const
    {
        return providers;
    }

    const std::vector<int> &getCustomers() const
    {
        return customers;
    }

    const std::vector<int> &getPeers() const
    {
        return peers;
    }
};