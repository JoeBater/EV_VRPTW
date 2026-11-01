#pragma once

#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <string>

// Locations of the solver's inputs (data/) and outputs (results/).
//
// Resolution order for each root:
//   1. set explicitly (main.cpp --data / --results)
//   2. environment variable EV_VRPTW_DATA / EV_VRPTW_RESULTS
//   3. compile-time default from CMake: <repo>/data, <repo>/results

#ifndef EV_VRPTW_DEFAULT_DATA_DIR
#define EV_VRPTW_DEFAULT_DATA_DIR "data"
#endif
#ifndef EV_VRPTW_DEFAULT_RESULTS_DIR
#define EV_VRPTW_DEFAULT_RESULTS_DIR "results"
#endif

namespace paths {

namespace fs = std::filesystem;

inline std::string& dataOverride()    { static std::string s; return s; }
inline std::string& resultsOverride() { static std::string s; return s; }

inline void setDataRoot(const std::string& d)    { dataOverride() = d; }
inline void setResultsRoot(const std::string& r) { resultsOverride() = r; }

inline fs::path resolveRoot(const std::string& override, const char* envVar, const char* fallback)
{
    if (!override.empty()) return override;
    if (const char* env = std::getenv(envVar)) return env;
    return fallback;
}

inline fs::path dataRoot()    { return resolveRoot(dataOverride(),    "EV_VRPTW_DATA",    EV_VRPTW_DEFAULT_DATA_DIR); }
inline fs::path resultsRoot() { return resolveRoot(resultsOverride(), "EV_VRPTW_RESULTS", EV_VRPTW_DEFAULT_RESULTS_DIR); }

// ---- inputs (produced by dataprep/) ----

inline std::string instanceFile(const std::string& city, int scale, const std::string& benchmark)
{
    return (dataRoot() / "processed" / "instances" /
            (city + "_" + std::to_string(scale) + "_" + benchmark + ".csv")).string();
}

inline std::string shortPathsFile(const std::string& benchmark)
{
    return (dataRoot() / "processed" / "short_paths" / (benchmark + "_short_paths.txt")).string();
}

inline std::string weatherFile(const std::string& city)
{
    return (dataRoot() / "weather" / "open-meteo" / (city + "-weather.csv")).string();
}

// True if every input for (city, scale, benchmark) exists; reports what is missing.
inline bool inputsExist(const std::string& city, int scale, const std::string& benchmark)
{
    bool ok = true;
    for (const auto& f : {instanceFile(city, scale, benchmark), shortPathsFile(benchmark), weatherFile(city)}) {
        if (!fs::exists(f)) {
            std::cerr << "missing input: " << f << "\n";
            ok = false;
        }
    }
    return ok;
}

// ---- outputs ----

// results/<sub>/, created if needed, returned with a trailing separator
inline std::string resultsDir(const fs::path& sub)
{
    fs::path dir = resultsRoot() / sub;
    fs::create_directories(dir);
    return dir.string() + "/";
}

} // namespace paths
