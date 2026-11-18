#pragma once

#include <string>
#include <vector>

// The Solomon benchmarks used by the experiments, with the fleet each is solved with.
//
// capacity: Solomon's vehicle capacity (200 for the 1-series, 1000 for r2/rc2).
// vehicles: the best-known Solomon vehicle count, the fleet exp0 starts from. The LNS keeps
//           every route non-empty, so the fleet is fixed per run; exp0 adds vehicles until
//           the flatland solution has no late stops or overload. Travel here is faster than
//           Solomon's (1 unit = 50 m at 25-55 km/h, not 1 unit per minute).
struct Benchmark {
    std::string name;
    int vehicles;
    int capacity;
};

inline const std::vector<Benchmark>& benchmarks()
{
    static const std::vector<Benchmark> list = {
        {"c107", 10, 200}, {"c108", 10, 200}, {"c109", 10, 200},
        {"r101", 19, 200}, {"r104",  9, 200}, {"r105", 14, 200},
        {"r204",  2, 1000}, {"r205", 3, 1000}, {"r208", 2, 1000},
        {"rc101", 14, 200}, {"rc104", 10, 200}, {"rc105", 13, 200}, {"rc107", 11, 200},
        {"rc201",  4, 1000}, {"rc204", 3, 1000}, {"rc207", 3, 1000},
    };
    return list;
}

// Every city with generated inputs; cities without inputs are skipped at run time.
inline const std::vector<std::string>& cities()
{
    static const std::vector<std::string> list = {
        "Aberystwyth", "Bath", "Belfast", "Birmingham", "Bristol", "Cambridge", "Cardiff",
        "Carlisle", "Chichester", "Derry", "Glasgow", "Inverness", "Lancaster", "Leeds",
        "Liverpool", "London", "Newcastle", "Plymouth", "Southampton", "Wrexham",
    };
    return list;
}
