#pragma once
#include <string>
#include <vector>
#include <memory>

#include "Policy.h"
#include "BGP.h"
#include "ROV.h"

using std::string, std::vector, std::unique_ptr, std::make_unique;

class AS
{
private:
    int asn;
    vector<int> providers;
    vector<int> customers;
    vector<int> peers;
    unique_ptr<Policy> policy;
public:
    AS(int asn, bool useROV = false) : asn(asn)
    {
        if (useROV)
        {
            policy = make_unique<ROV>(asn);
        }
        else
        {
            policy = make_unique<BGP>(asn);
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

    const vector<int> &getProviders() const
    {
        return providers;
    }

    const vector<int> &getCustomers() const
    {
        return customers;
    }

    const vector<int> &getPeers() const
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
