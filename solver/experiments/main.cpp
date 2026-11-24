#include <iostream>
#include <string>
#include <vector>

#include "Instance.hpp"
#include "LNS.hpp"
#include "ProjectPaths.hpp"

#include "exp0.hpp"
#include "exp1.hpp"

static void usage()
{
    std::cout << "usage: ev_vrptw [--exp 0|0a|1|1a|1b|1c] [--bench NAME]... [--iterations N] [--random-start] [--detailed] [--year YYYY] [--start-hour H] [--no-regen] [--data DIR] [--results DIR]\n"
              << "  --exp        0 = solve the flatland baselines on the MST (default); 0a = on the complete graph;\n"
              << "               re-simulate the MST baselines in every city with\n"
              << "               1a = terrain only, 1b = weather only (every day), 1c = both; 1 = all three\n"
              << "  --bench      only this benchmark (repeatable; default all)\n"
              << "  --iterations LNS iterations per benchmark, for exp 0 (default 250000)\n"
              << "  --random-start exp 0/0a: start from a random assignment, not a published or smaller solution\n"
              << "  --detailed   exp 1: also write per-minute route logs (~100 MB per city per benchmark)\n"
              << "  --start-hour clock hour of minute 0, for the weather (default 0 = midnight)\n"
              << "  --no-regen   disable regenerative braking\n"
              << "  --year     weather year (default " << paths::weatherYear() << ")\n"
              << "  --data     data root    (default " << paths::dataRoot().string() << ")\n"
              << "  --results  results root (default " << paths::resultsRoot().string() << ")\n";
}

int main(int argc, char* argv[])
{
    std::string experiment = "0";
    bool detailed = false;
    bool randomStart = false;
    int iterations = 250000;
    std::vector<std::string> only;

    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if ((arg == "--data" || arg == "--results" || arg == "--exp" || arg == "--year" || arg == "--start-hour" ||
             arg == "--bench" || arg == "--iterations") && i + 1 < argc) {
            std::string value = argv[++i];
            if (arg == "--data")    paths::setDataRoot(value);
            if (arg == "--results") paths::setResultsRoot(value);
            if (arg == "--exp")     experiment = value;
            if (arg == "--year")    paths::setWeatherYear(std::stoi(value));
            if (arg == "--start-hour") Route::setDayStartHour(std::stoi(value));
            if (arg == "--bench")   only.push_back(value);
            if (arg == "--iterations") iterations = std::stoi(value);
        } else if (arg == "--random-start") {
            randomStart = true;
        } else if (arg == "--detailed") {
            detailed = true;
        } else if (arg == "--no-regen") {
            Route::setRegenBraking(false);
        } else {
            usage();
            return arg == "--help" || arg == "-h" ? 0 : 1;
        }
    }

    std::cout << "data:    " << paths::dataRoot().string() << "\n"
              << "results: " << paths::resultsRoot().string() << "\n"
              << "weather: " << paths::weatherYear() << "\n"
              << "regen:   " << (Route::regenBraking() ? "on" : "off") << "\n"
              << "start:   " << Route::dayStartHour() << ":00\n";

    if (experiment == "0") {
        exp0 experiment0(only, iterations, false, randomStart);
    } else if (experiment == "0a") {
        exp0 experiment0a(only, iterations, true, randomStart);
    } else if (experiment == "1") {
        for (const char* mode : {"1a", "1b", "1c"}) exp1 experiment1(mode, only, detailed);
    } else if (experiment == "1a" || experiment == "1b" || experiment == "1c") {
        exp1 experiment1(experiment, only, detailed);
    } else {
        usage();
        return 1;
    }

	return 0;
}
