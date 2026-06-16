#ifndef ROUTING_TABLE_H
#define ROUTING_TABLE_H

#include "trie.h"

template <typename Type>
class RoutingTable
{
private:
    Trie<Type> trie;

public:
    class Prefix
    {
    public:
        Type address;
        int len;
    };

    RoutingTable(int w) : trie(w) {}
    void add_route(Prefix prefix, int value);
    void remove_route(Prefix prefix);
    int lookup(Type address);
};

#include "routing_table.tpp"
#endif