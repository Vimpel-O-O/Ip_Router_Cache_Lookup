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
    std::unordered_map<Type, int> hmap;
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