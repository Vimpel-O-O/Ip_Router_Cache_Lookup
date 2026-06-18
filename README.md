# IP Router Cache Lookup

Demo: [YouTube link](https://www.youtube.com/watch?v=3ZArGDpWC5Y)

A small C++ project that mimics how a router decides where to send a packet, and
then speeds it up with a cache. Works for both **IPv4** and **IPv6**.

## The idea

A router gets a packet with a destination IP and has to pick which port to send it
out of. It keeps a **routing table** of rules like "addresses in this group go to
port 5". The tricky part is that one IP can match many rules, so the router picks the
**most specific** one — this is called **Longest Prefix Match (LPM)**.

Doing LPM for every packet is slow, but real traffic repeats a lot. So I put a
**cache** in front of the table: the first time I look up an IP I do the slow LPM and
remember the answer; every time after that I just read it back instantly.

Also, when a route is added or removed, some cached answers become
wrong (stale). So on any route change I throw away only the cache entries that the
change could affect.

## How it works

Routes:

```
0.0.0.0/0       -> port 1   (default, "everything else")
192.168.0.0/16  -> port 4
192.168.1.0/24  -> port 5   (more specific)
```

Look up `192.168.1.5`:

- `/0`, `/16`, and `/24` all match → longest wins → **port 5**.

Now remove `192.168.1.0/24`. The cached answer for `192.168.1.5` is now stale, so it
gets invalidated. The next lookup falls back to `/16` → **port 4**.

## Implementation Architecture

<img width="2924" height="2980" alt="image" src="https://github.com/user-attachments/assets/4fbfcd02-2478-4c97-98dd-aad9a8eac112" />

- **Trie** ([trie.h](trie.h) / [trie.tpp](trie.tpp))
  A bit-by-bit binary tree. Walking the address bits down the tree and remembering the
  deepest marked node gives the longest prefix match.
- **RoutingTable** ([routing_table.h](routing_table.h) / [routing_table.tpp](routing_table.tpp))
  A thin wrapper over the trie which uses `Prefix{address, len}` and calls
  `add_route` / `remove_route` / `lookup`.
- **RouteCache** ([route_cache.h](route_cache.h) / [route_cache.tpp](route_cache.tpp))
  Uses an `unordered_map<IP, port>` in front of the table. Checks the cache first, fills on
  a miss, and on route changes calls `invalidate()` to drop only the entries under the
  changed prefix. Also tracks hit rate.

Everything is a template on the address type, and a custom hash was implemented for 128-bit IP, so the same code runs for IPv4
(`uint32_t`) and IPv6 (`unsigned __int128`).

## Cached vs Uncached Benchmark Results ([main.cpp](main.cpp))

1. Total Lookups: 1000004
2. Cache Hit Rate: 90.0053 %
3. With Cache: 14.2342 ms
4. Without Cache: 21.4246 ms
5. Speedup: 1.50515x

## Build & run

```bash
g++ -std=c++17 -O2 main.cpp -o router_sim
./router_sim
```
