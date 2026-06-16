# IP Router Cache Lookup

A small C++ simulator that models how a real router forwards packets, then speeds it
up with a **lookaside cache** — and handles the hard part: keeping that cache correct
when the network changes.

This README is the single source of context. If you just cloned the repo, read it
top to bottom and you'll understand the whole project and where your work fits.

---

## 1. What we're building (one paragraph)

A router receives packets, each stamped with a destination **IP address**, and must
decide where to send each one next. It does this with a **routing table** that stores
**routes** (rules for groups of addresses) and finds the **most specific** matching
rule — **Longest Prefix Match (LPM)** — using a **trie**. Because that lookup is
expensive and traffic repeats, we put a **cache** (a hashmap of exact-address →
answer) in front of it. The interesting problem: when routes are added or dropped,
cached answers can go **stale**, so we must **invalidate** them. We then run simulated
traffic through the system and measure **hit rate** and **lookup speed**.

---

## 2. Concepts / glossary

```
IP address  = the destination written on the envelope (ONE machine).
Prefix      = a GROUP of addresses, written address/length, e.g. 192.168.1.0/24.
              The "/24" = how many leading bits are fixed = how big the group is.
Route       = a RULE: prefix -> interface + next-hop ("for these, send THIS way").
Interface   = the physical wire a packet leaves on.
Next-hop    = the neighbor router to hand it to (one step closer to the destination).

LPM         = among matching prefixes, follow the MOST SPECIFIC (longest) rule.
              About being CORRECT, not fast.
Metric/cost = among routes to the SAME place, pick the FASTEST path.
              A SEPARATE mechanism (out of scope) — this is where bandwidth lives.

Trie        = bit-by-bit tree of prefixes; walking an address down it gives LPM
              "for free" (deepest marked node = longest match).
Cache       = exact-address -> answer hashmap; check first, fill on miss. NO prefixes.
Consistency = throwing away stale cache entries when routes change.
```

Key mental model:

- The **trie thinks in prefixes/specificity** (LPM). The **cache thinks in exact
  addresses only**. A cache entry goes stale only when a **route changes**, never
  because of a "similar" address.
- A prefix is stored as **(address value, length)** — one number plus a length — not
  as four octets. Octets are just human formatting.
- Each router makes ONE local decision (LPM -> next-hop). The full internet path
  EMERGES from every router doing this independently.

---

## 3. The flow (one worked example)

Routing table with two rules:

```
Rule A:  0.0.0.0/0       -> ISP           ("everything else" fallback)
Rule B:  192.168.1.0/24  -> local switch  ("my street")
```

Packet arrives for `192.168.1.5`:

```
1. Check cache            -> MISS (never seen this address)
2. Trie LPM lookup        -> /0 matches, /24 matches; longest wins -> Rule B -> local switch
3. Store in cache         -> 192.168.1.5 -> local switch
4. Forward packet
```

Second packet for `192.168.1.5`:

```
1. Check cache            -> HIT -> local switch (skip the trie entirely)
```

Topology shift (the consistency problem):

```
- Admin adds 192.168.1.128/25 -> port 9
- A fresh lookup for 192.168.1.130 would now pick the /25, not the /24
- But the cache still holds the old answer -> STALE
- So on route change we INVALIDATE affected cache entries -> next lookup recomputes
```

---

## 4. Architecture / components

```
            +-------------------+
 packets -> |  Simulator (main) |  feeds destination IPs, drives route changes
            +---------+---------+
                      |
                      v
            +-------------------+      miss      +-------------------+
            |      Cache        | -------------> |   Routing Table   |
            | exact IP -> answer | <------------ |   (Trie / LPM)    |
            +---------+---------+   fill answer  +-------------------+
                      ^                                   |
                      | invalidate(prefix) on change      | add_route / remove_route
                      +-----------------------------------+
                      |
            +-------------------+
            |      Metrics      |  hits, misses, hit rate, timings
            +-------------------+
```

- **Routing Table (Trie):** source of truth. `add_route`, `remove_route`, `lookup`.
- **Cache:** lookaside hashmap in front of the trie. Check-first, fill-on-miss,
  invalidate on route change.
- **Simulator/driver:** generates the workload, injects route changes, wires
  everything together.
- **Metrics:** counts and timings -> final report.

---

## 5. Interface contract (agree on this FIRST)

This is the seam that lets us work in parallel. Names/types are a starting point —
refine together, but lock it before implementing. Likely lives in a shared header
(e.g. `types.hpp` / `routing_table.hpp` / `cache.hpp`).

```cpp
// --- shared address/result types ---
using IpV4 = uint32_t;                 // 32-bit address; IPv6 comes later

struct LookupResult {
    int      interface;                // which port to send out
    IpV4     next_hop;                 // neighbor to hand it to
    bool     found;                    // false = no matching route (drop)
};

// --- Routing Table (Trie)  [owner: Person A] ---
class RoutingTable {
public:
    void add_route(IpV4 prefix, uint8_t length, LookupResult answer);
    void remove_route(IpV4 prefix, uint8_t length);
    LookupResult lookup(IpV4 address) const;   // does LPM, no caching
};

// --- Cache  [owner: Person B] ---
class RouteCache {
public:
    explicit RouteCache(RoutingTable& table);
    LookupResult lookup(IpV4 address);         // check cache, else table + fill
    void invalidate(IpV4 prefix, uint8_t length); // drop entries under this prefix
    void clear();                              // full flush (simple fallback)
    // metrics hooks: hits(), misses(), hit_rate()
};
```

Coordination rule: when a route changes, the driver (or the table) must notify the
cache via `invalidate(...)` (or `clear()` for the simple version). That callback is
the contract between the two halves.

---

## 6. Task division

**Person A — Routing Table / Trie (the LPM engine)**

- Address representation + parsing (`192.168.1.5` <-> `uint32_t`, bit access helpers).
- Binary trie: node layout, `add_route`, `remove_route`.
- `lookup` doing Longest Prefix Match (deepest-marked-node walk).
- Unit tests for LPM correctness (overlapping prefixes, default route, no-match).

**Person B — Cache + Simulation + Metrics (everything around the engine)**

- `RouteCache`: hashmap wrapper, check-first / fill-on-miss.
- Consistency: `invalidate(prefix, length)` (start with `clear()`, then selective).
- Workload generator: stream of destination IPs **with locality** (so the cache hits).
- Simulator/driver `main`: wire table + cache, inject route changes mid-workload.
- Metrics + final report: hits, misses, hit rate, cached-vs-uncached timing.

**Shared / do together first**

- Lock the **interface contract** in §5 (the header).
- Decide the build setup (§7).

---

## 7. Build & run

> TODO once code exists. Suggested simple setup:

```bash
# clone
git clone <repo-url>
cd IpRouterCacheLookup_project

# build (placeholder — pick CMake or a Makefile together)
cmake -B build && cmake --build build      # or: make

# run the simulator
./build/router_sim                          # or: ./router_sim
```

Target: C++17. One executable that runs a workload and prints the metrics report.

---

## 8. Roadmap / phases

1. **IPv4 core** — trie + LPM, cache, full-flush invalidation, workload, metrics.
2. **Selective invalidation** — drop only cache entries under a changed prefix
   (the real "consistency" challenge), instead of flushing everything.
3. **IPv6** — widen the address to 128 bits (`std::array<uint8_t,16>`); the core
   algorithm should not change if the address type was abstracted well.
4. **Optimization (optional)** — compressed trie (multibit / Patricia), richer
   workloads, more metrics (p50/p99 latency).

Sequencing rule: get IPv4 working end-to-end **before** generalizing to IPv6.

---

## 9. Working with agents

Each teammate can run their own Claude Code agents against their slice. To keep work
mergeable:

- Implement to the **interface contract in §5**; don't change shared signatures
  without telling the other person.
- Keep your work on a branch; small PRs per component.
- Tests live next to the component they cover.

```

```
