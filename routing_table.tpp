#include "routing_table.h"

template <typename Type>
void RoutingTable<Type>::add_route(Prefix prefix, int value)
{
    trie.insert(prefix.address, prefix.len, value);
}

template <typename Type>
void RoutingTable<Type>::remove_route(Prefix prefix)
{
    trie.remove(prefix.address, prefix.len);
}

template <typename Type>
int RoutingTable<Type>::lookup(Type address)
{
    return trie.LPM(address);
}