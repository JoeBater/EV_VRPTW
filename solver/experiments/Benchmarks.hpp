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

// The Solomon benchmarks extended with added stores (dataprep extend.py): <name>_<customers>.
// Each size contains the one before. vehicles is the Solomon fleet scaled by size, only used when
// the previous size has no baseline to start from (see exp0).
inline const std::vector<int>& extendedSizes()
{
    static const std::vector<int> sizes = {200, 400};
    return sizes;
}

inline const std::vector<Benchmark>& benchmarks()
{
    static const std::vector<Benchmark> list = [] {
        std::vector<Benchmark> solomon = {
            {"c107", 10, 200}, {"c108", 10, 200}, {"c109", 10, 200},
            {"r101", 19, 200}, {"r104",  9, 200}, {"r105", 14, 200},
            {"r204",  2, 1000}, {"r205", 3, 1000}, {"r208", 2, 1000},
            {"rc101", 14, 200}, {"rc104", 10, 200}, {"rc105", 13, 200}, {"rc107", 11, 200},
            {"rc201",  4, 1000}, {"rc204", 3, 1000}, {"rc207", 3, 1000},
        };
        std::vector<Benchmark> all = solomon;
        for (int size : extendedSizes())
            for (const Benchmark& b : solomon)
                all.push_back({b.name + "_" + std::to_string(size), b.vehicles * size / 100, b.capacity});
        return all;
    }();
    return list;
}

// "r101_400" -> "r101_200", "r101_200" -> "r101", "r101" -> ""
inline std::string previousSize(const std::string& name)
{
    size_t underscore = name.rfind('_');
    if (underscore == std::string::npos) return "";
    int size = std::stoi(name.substr(underscore + 1));
    std::string root = name.substr(0, underscore);
    int previous = 100;
    for (int s : extendedSizes()) if (s < size) previous = std::max(previous, s);
    return previous == 100 ? root : root + "_" + std::to_string(previous);
}

inline int customerCount(const std::string& name)
{
    size_t underscore = name.rfind('_');
    return underscore == std::string::npos ? 100 : std::stoi(name.substr(underscore + 1));
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
