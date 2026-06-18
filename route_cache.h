#ifndef ROUTE_CACHE_H
#define ROUTE_CACHE_H
#include <unordered_map>
#include <stdexcept>

#include "routing_table.h"

template <typename Type>
class RouteCache
{
private:
    int hit = 0;
    int total = 0;

    // hash that works for BOTH uint32_t (IPv4) and unsigned __int128 (IPv6)
    struct IpHash
    {
        std::size_t operator()(Type x) const
        {
            if constexpr (sizeof(Type) <= 8)
            {
                return std::hash<Type>{}(x);
            }
            else
            {
                // size_t = 64 bits by default
                // spliting into two 64-bit halves and mix them
                std::uint64_t bottom = static_cast<std::uint64_t>(x);
                std::uint64_t top = static_cast<std::uint64_t>(x >> 64);
                std::size_t seed = std::hash<std::uint64_t>{}(bottom);
                seed ^= std::hash<std::uint64_t>{}(top) + 0x9e3779b97f4a7c15ULL + (seed << 6) + (seed >> 2);
                return seed;
            }
        }
    };

    std::unordered_map<Type, int, IpHash> hmap;
    RoutingTable<Type> &table;

public:
    RouteCache(RoutingTable<Type> &t) : table(t) {}
    int lookup(Type address);
    void add_route(typename RoutingTable<Type>::Prefix prefix, int value);
    void remove_route(typename RoutingTable<Type>::Prefix prefix);
    void invalidate(Type address, int len);
    void clear();
    double getHitRate();
    int getTotalLookups();
};

#include "route_cache.tpp"
#endif