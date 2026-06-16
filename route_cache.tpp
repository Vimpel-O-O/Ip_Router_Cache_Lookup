#include "route_cache.h"

template <typename Type>
int RouteCache<Type>::lookup(Type address)
{
    total++;
    auto iterator = hmap.find(address);
    if (iterator != hmap.end())
    {
        hit++;
        return iterator->second;
    }
    else
    {
        int prefix = table.lookup(address);
        hmap[address] = prefix;
        return prefix;
    }
}

template <typename Type>
void RouteCache<Type>::add_route(typename RoutingTable<Type>::Prefix prefix, int value)
{
    table.add_route(prefix, value);
    invalidate(prefix.address, prefix.len);
}

template <typename Type>
void RouteCache<Type>::remove_route(typename RoutingTable<Type>::Prefix prefix)
{
    table.remove_route(prefix);
    invalidate(prefix.address, prefix.len);
}

template <typename Type>
void RouteCache<Type>::invalidate(Type address, int len)
{
    // Build Mask
    Type mask;
    if (len == 0)
    {
        mask = 0;
    }
    else if (len >= 32)
    {
        mask = 0xFFFFFFFF;
    }
    else
    {
        mask = 0xFFFFFFFFu << (32 - len);
    }

    // Prefix's Network
    Type prefix = address & mask;

    // Scan and erase matches
    auto iterator = hmap.begin();
    while (iterator != hmap.end())
    {
        if ((iterator->first & mask) == prefix)
        {
            iterator = hmap.erase(iterator);
        }
        else
        {
            ++iterator;
        }
    }
}

template <typename Type>
void RouteCache<Type>::clear()
{
    hmap.clear();
}

template <typename Type>
double RouteCache<Type>::getHitRate()
{
    return total == 0 ? 0.0 : (double)hit / total;
}

template <typename Type>
int RouteCache<Type>::getTotalLookups()
{
    return total;
}