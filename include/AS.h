#pragma once
#include <string>
#include <vector>
#include <memory>

#include "Policy.h"
#include "BGP.h"
#include "ROV.h"

class AS
{
private:
    int asn;
    std::vector<int> providers;
    std::vector<int> customers;
    std::vector<int> peers;
    std::unique_ptr<Policy> policy;

public:
    AS(int asn, bool useROV = false) : asn(asn)
    {
        if (useROV)
        {
            policy = std::make_unique<ROV>(asn);
        }
        else
        {
            policy = std::make_unique<BGP>(asn);
        }
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

    Policy &getPolicy()
    {
        return *policy;
    }

    const Policy &getPolicy() const
    {
        return *policy;
    }
};
