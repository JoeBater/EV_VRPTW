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
inline int& weatherYearSetting()       { static int y = 2024; return y; }

inline void setDataRoot(const std::string& d)    { dataOverride() = d; }
inline void setResultsRoot(const std::string& r) { resultsOverride() = r; }
inline void setWeatherYear(int y)                { weatherYearSetting() = y; }
inline int  weatherYear()                        { return weatherYearSetting(); }

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

// ~100 m terrain profile along each MST edge; optional - without it the solver
// uses one slope per edge from the site elevations
inline std::string edgeProfileFile(const std::string& city, int scale, const std::string& benchmark)
{
    std::string file = (dataRoot() / "processed" / "edge_profiles" /
                        (city + "_" + std::to_string(scale) + "_" + benchmark + ".csv")).string();
    if (!fs::exists(file)) {
        std::cerr << "no edge profile (using site-to-site slopes): " << file << "\n";
        return "";
    }
    return file;
}

// Hourly weather for the selected year (main.cpp --year, default 2024)
inline std::string weatherFile(const std::string& city)
{
    std::string year = std::to_string(weatherYear());
    return (dataRoot() / "weather" / "open-meteo" / year / (city + "-weather-" + year + ".csv")).string();
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
