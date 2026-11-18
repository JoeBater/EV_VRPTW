#include "exp1.hpp"

#include <algorithm>
#include <fstream>

#include "Benchmarks.hpp"
#include "ProjectPaths.hpp"

static const int DAYS_IN_MONTH[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
static const double FLAT_TEMPERATURE_C = 20.0;  // as in exp0

// Re-simulates the flatland baseline solutions (from exp0) in every city, without
// re-optimising, to measure what the real world does to them:
//   1a  the city's terrain, flatland weather (20 C, no rain)   - one run per city
//   1b  the city's weather for every day of the year, flat terrain
//   1c  the city's terrain and weather for every day of the year
//
// Writes results/solutions/flatland_<scale>/<benchmark>/exp1<mode><variant>.csv, one row per
// city and day (month = day = 0 for 1a), plus a "flatland" row re-simulating the baseline
// itself as the reference. A route is depleted when its charge drops below 0 at any minute;
// a run is feasible when no stop or depot return is late and no route is depleted.
// With detailed = true it also writes each run's per-minute log to <City><variant>_<mode>/.
exp1::exp1(const std::string& mode, const std::vector<std::string>& only, bool detailed)
{
    const int scale = 5;
    const bool terrain = mode == "1a" || mode == "1c";
    const bool weather = mode == "1b" || mode == "1c";

    // Variant runs go to exp1c_noregen.csv, exp1c_start08.csv etc. beside the default run
    string variant = Route::regenBraking() ? "" : "_noregen";
    if (Route::dayStartHour() != 0) {
        char start[16];
        std::snprintf(start, sizeof(start), "_start%02d", Route::dayStartHour());
        variant += start;
    }

    for (const Benchmark& b : benchmarks()) {
        if (!only.empty() && std::find(only.begin(), only.end(), b.name) == only.end()) continue;

        string solutionDir = paths::resultsDir("solutions/flatland_" + std::to_string(scale) + "/" + b.name);
        string solutionFile = solutionDir + "route_log.txt";
        if (!std::filesystem::exists(solutionFile)) {
            cerr << "skipping " << b.name << ": no baseline at " << solutionFile << " (run exp0 first)" << endl;
            continue;
        }

        std::ofstream out(solutionDir + "exp" + mode + variant + ".csv");
        out << "benchmark,mode,city,month,day,routes,distance_m,late_stops,tardiness_min,"
               "depleted_routes,min_soc,mean_final_soc,max_soc_used,feasible\n";

        auto record = [&](Instance& instance, const string& city, int month, int day) {
            int routes = 0, depleted = 0;
            double minSoc = 1.0, sumFinal = 0.0, maxUsed = 0.0;
            for (const Route& r : instance.getRoutes()) {
                vector<SoCEntry> detail = r.getSoCDetail();
                if (detail.empty()) continue;
                double routeMin = 1.0;
                for (const SoCEntry& e : detail) routeMin = std::min(routeMin, e.cumulative_soc);
                double final = detail.back().cumulative_soc;
                routes++;
                if (routeMin < 0.0) depleted++;
                minSoc = std::min(minSoc, routeMin);
                sumFinal += final;
                maxUsed = std::max(maxUsed, 1.0 - final);
            }
            int late = instance.getTotalLate();
            out << b.name << "," << mode << "," << city << "," << month << "," << day << ","
                << routes << "," << instance.getTotalDistance() << "," << late << ","
                << instance.getTotalTardiness() << "," << depleted << "," << minSoc << ","
                << (routes ? sumFinal / routes : 0.0) << "," << maxUsed << ","
                << (late == 0 && depleted == 0 ? 1 : 0) << "\n";
        };

        auto load = [&](const string& city) {
            Instance* instance = new Instance(paths::instanceFile(city, scale, b.name),
                                              paths::shortPathsFile(b.name),
                                              paths::weatherFile(city),
                                              b.capacity, b.vehicles,
                                              paths::edgeProfileFile(city, scale, b.name));
            instance->setVerbosity(0);
            instance->setDebug(0);
            instance->readRoutesFromFile(solutionFile);
            return instance;
        };

        // reference: the baseline itself, as exp0 simulated it
        {
            Instance* flat = load("Cambridge");
            flat->getTodaysWeather(1, 1);
            flat->flattenWeather(FLAT_TEMPERATURE_C);
            flat->flattenCity();
            flat->calculateRoutes();
            record(*flat, "flatland", 0, 0);
            delete flat;
        }

        for (const string& city : cities()) {
            if (!paths::inputsExist(city, scale, b.name)) {
                cerr << "skipping " << city << " (generate inputs with dataprep)" << endl;
                continue;
            }
            cout << "== exp" << mode << ": " << b.name << " in " << city << variant << endl;

            Instance* instance = load(city);
            if (!terrain) instance->flattenCity();
            string cityDir = detailed
                ? paths::resultsDir("solutions/flatland_" + std::to_string(scale) + "/" + b.name + "/" + city + variant + "_" + mode)
                : "";

            if (!weather) {
                instance->getTodaysWeather(1, 1);
                instance->flattenWeather(FLAT_TEMPERATURE_C);
                instance->calculateRoutes();
                record(*instance, city, 0, 0);
                if (detailed) instance->logDetailedRoutes(cityDir + "detailed_route_log.csv");
            } else {
                for (int month = 1; month <= 12; month++) {
                    for (int day = 1; day <= DAYS_IN_MONTH[month - 1]; day++) {
                        instance->getTodaysWeather(month, day);
                        instance->calculateRoutes();
                        record(*instance, city, month, day);
                        if (detailed) {
                            instance->logDetailedRoutes(cityDir + "detailed_route_log_" + std::to_string(month) + "_" + std::to_string(day) + ".csv");
                        }
                    }
                }
            }
            out.flush();
            delete instance;
        }
    }
}

exp1::~exp1()
{
}
