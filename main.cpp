// main.cpp — IP Router Cache Lookup simulator / demo
// RouteCache -> RoutingTable -> Trie (LPM)

#include <cstdint>
#include <iostream>
#include <vector>
#include <random>
#include <chrono>
#include "route_cache.h"
using namespace std;

// Build a 32-bit IPv4
static uint32_t ip(uint32_t a, uint32_t b, uint32_t c, uint32_t d)
{
    return (a << 24) | (b << 16) | (c << 8) | d;
}

// Build a 128-bit IPv6
using u128 = unsigned __int128;
static u128 make_v6(uint64_t top, uint64_t bot)
{
    return ((u128)top << 64) | bot;
}

static void demo_v6()
{
    using TableV6 = RoutingTable<u128>;
    using PrefixV6 = TableV6::Prefix;

    TableV6 table(128); // 128-bit (IPv6) routing table
    RouteCache<u128> cache(table);

    // IPv6 routes (prefix -> port)
    cache.add_route(PrefixV6{make_v6(0, 0), 0}, 1);                      // ::/0
    cache.add_route(PrefixV6{make_v6(0x20010db800000000ULL, 0), 32}, 2); // 2001:db8::/32
    cache.add_route(PrefixV6{make_v6(0x20010db800010000ULL, 0), 48}, 3); // 2001:db8:1::/48

    cout << "===IPv6 Lookup correctness===\n";
    cout << "2001:db8:1::5 -> port "
         << cache.lookup(make_v6(0x20010db800010000ULL, 5)) << "  (Expected 3)\n";
    cout << "2001:db8:9::9 -> port "
         << cache.lookup(make_v6(0x20010db800090000ULL, 9)) << "  (Expected 2)\n";
    cout << "2002::1       -> port "
         << cache.lookup(make_v6(0x2002000000000000ULL, 1)) << "  (Expected 1)\n\n";

    cout << "===IPv6 Cache===\n";
    cache.lookup(make_v6(0x20010db800010000ULL, 5)); // repeat to cache HIT
    cout << "Total Lookups: " << cache.getTotalLookups() << "\n";
    cout << "Cache Hit Rate: " << (cache.getHitRate() * 100.0) << " %\n\n";

    cout << "===IPv6 Consistency on route list change===\n";
    cout << "before remove: 2001:db8:1::5 -> port "
         << cache.lookup(make_v6(0x20010db800010000ULL, 5)) << "  (Expected 3)\n";
    cache.remove_route(PrefixV6{make_v6(0x20010db800010000ULL, 0), 48});
    cout << "after  remove: 2001:db8:1::5 -> port "
         << cache.lookup(make_v6(0x20010db800010000ULL, 5)) << "  (Expected 2)\n\n";
}

int main()
{
    using TableV4 = RoutingTable<uint32_t>;
    using PrefixV4 = TableV4::Prefix;

    TableV4 table(32);
    RouteCache<uint32_t> cache(table);

    // 1) Install some routes
    cache.add_route(PrefixV4{ip(0, 0, 0, 0), 0}, 8);      // 0.0.0.0/0
    cache.add_route(PrefixV4{ip(10, 0, 0, 0), 8}, 2);     // 10.0.0.0/8
    cache.add_route(PrefixV4{ip(172, 16, 0, 0), 12}, 3);  // 172.16.0.0/12
    cache.add_route(PrefixV4{ip(192, 168, 0, 0), 16}, 4); // 192.168.0.0/16
    cache.add_route(PrefixV4{ip(192, 168, 1, 0), 24}, 5); // 192.168.1.0/24

    // 2) Long Prefix Match check
    cout << "===IPv4 Lookup correctness===\n";
    cout << "192.168.1.5  -> port " << cache.lookup(ip(127, 0, 0, 1)) << "  (Expected 5)\n";
    cout << "192.168.9.9  -> port " << cache.lookup(ip(192, 168, 9, 9)) << "  (Expected 4)\n";
    cout << "10.1.2.3     -> port " << cache.lookup(ip(10, 1, 2, 3)) << "  (Expected 2)\n";
    cout << "8.8.8.8      -> port " << cache.lookup(ip(8, 8, 8, 8)) << "  (Expected 1)\n\n";

    // 3) Network traffic workload
    const int N = 1'000'000;

    // locality simulation
    const uint32_t hot[] = {
        ip(192, 168, 1, 5),
        ip(192, 168, 1, 99),
        ip(192, 168, 9, 9),
        ip(10, 1, 2, 3),
        ip(172, 16, 4, 4),
        ip(8, 8, 8, 8),
    };
    const int HOT = sizeof(hot) / sizeof(hot[0]);

    mt19937 rng(12345); // fixed seed for clear benchmark
    uniform_int_distribution<int> random_hot(0, HOT - 1);
    uniform_int_distribution<uint32_t> random_ip(0, 0xFFFFFFFFu);
    uniform_int_distribution<int> coin(0, 99);

    vector<uint32_t> workload;
    workload.reserve(N);
    for (int i = 0; i < N; ++i)
    {
        if (coin(rng) < 90)
            workload.push_back(hot[random_hot(rng)]);
        else
            workload.push_back(random_ip(rng));
    }

    // 4) Cache Workload
    long long sink = 0; // for compiler to not optimize the loops away
    auto start_time_cache = chrono::high_resolution_clock::now();
    for (uint32_t addr : workload)
    {
        sink += cache.lookup(addr);
    }
    auto end_time_cache = chrono::high_resolution_clock::now();
    double cached_ms = chrono::duration<double, milli>(end_time_cache - start_time_cache).count();

    // 5) Lookup from the trie (no cache)
    auto start_time_trie = chrono::high_resolution_clock::now();
    for (uint32_t addr : workload)
    {
        sink += table.lookup(addr);
    }
    auto end_time_trie = chrono::high_resolution_clock::now();
    double raw_ms = chrono::duration<double, milli>(end_time_trie - start_time_trie).count();

    cout << "=== Performance ===\n";
    cout << "Total Lookups: " << cache.getTotalLookups() << "\n";
    cout << "Cache Hit Rate: " << (cache.getHitRate() * 100.0) << " %\n";
    cout << "With Cache: " << cached_ms << " ms\n";
    cout << "Without Cache: " << raw_ms << " ms\n";
    cout << "Speedup: " << (raw_ms / cached_ms) << "x\n\n";

    // 6) Cache Consistency -> route change invalidates stale entries
    cout << "===Consistency on route change===\n";
    cout << "before remove: 192.168.1.5 -> port " << cache.lookup(ip(192, 168, 1, 5)) << "  (Expected 5)\n";
    cache.remove_route(PrefixV4{ip(192, 168, 1, 0), 24});
    cout << "after  remove: 192.168.1.5 -> port " << cache.lookup(ip(192, 168, 1, 5)) << "  (Expected 4)\n\n";

    demo_v6();

    cout << "(checksum: " << sink << ")\n"; // sink to be useful
    return 0;
}
