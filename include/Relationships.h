#pragma once

enum class Relationship
{
    ORIGIN = 4,
    CUSTOMER = 3,
    PEER = 2,
    PROVIDER = 1
};

enum class RelationshipType
{
    PEER_TO_PEER = 0,
    PROVIDER_TO_CUSTOMER = -1
};
