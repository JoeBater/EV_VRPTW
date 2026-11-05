#include <iostream>
#include <string>

#include "Instance.hpp"
#include "LNS.hpp"
#include "ProjectPaths.hpp"

#include "exp0.hpp"
#include "exp1.hpp"

static void usage()
{
    std::cout << "usage: ev_vrptw [--exp 0|1] [--year YYYY] [--start-hour H] [--no-regen] [--data DIR] [--results DIR]\n"
              << "  --exp        experiment to run (default 1)\n"
              << "  --start-hour clock hour of minute 0, for the weather (default 0 = midnight)\n"
              << "  --no-regen   disable regenerative braking\n"
              << "  --year     weather year (default " << paths::weatherYear() << ")\n"
              << "  --data     data root    (default " << paths::dataRoot().string() << ")\n"
              << "  --results  results root (default " << paths::resultsRoot().string() << ")\n";
}

int main(int argc, char* argv[])
{
    int experiment = 1;

    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if ((arg == "--data" || arg == "--results" || arg == "--exp" || arg == "--year" || arg == "--start-hour") && i + 1 < argc) {
            std::string value = argv[++i];
            if (arg == "--data")    paths::setDataRoot(value);
            if (arg == "--results") paths::setResultsRoot(value);
            if (arg == "--exp")     experiment = std::stoi(value);
            if (arg == "--year")    paths::setWeatherYear(std::stoi(value));
            if (arg == "--start-hour") Route::setDayStartHour(std::stoi(value));
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

    if (experiment == 0) {
        exp0 experiment0;
    } else if (experiment == 1) {
        exp1 experiment1;
    } else {
        usage();
        return 1;
    }

	return 0;
}
