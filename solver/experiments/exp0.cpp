#include "exp0.hpp"

#include <algorithm>
#include <chrono>
#include <ctime>
#include <fstream>
#include <sstream>

#include "Benchmarks.hpp"
#include "ProjectPaths.hpp"

// Baseline: solves every benchmark on flatland (no terrain, constant 20 C and no rain), the
// closest thing to the academic VRPTW. The site layout is the same in every city, so the
// committed Cambridge inputs are used and then flattened. exp1 re-simulates these solutions
// in each real city and day.
//
// exp0 drives every leg along the minimum spanning tree (MST), the network all later work
// uses. exp0a (completeGraph) drives every leg directly, on the complete Euclidean graph as in
// Solomon, to measure how much the MST alone changes the solutions; it also re-simulates each
// complete-graph solution on the flat MST (the mst_* columns). Terrain is never put on the
// complete graph.
//
// Writes, per benchmark, to results/solutions/<flatland|complete>_<scale>/<benchmark>/:
//   route_log.txt           the solution (exp1 reads the flatland one)
//   detailed_route_log.csv  its flatland simulation
//   searchProgress.csv      LNS progress
// and appends one row per attempt (fleet size) to .../baseline_summary.csv;
// the row with clean = 1 is the baseline.
//
// Warm start: if results/solomon_solutions/<benchmark>.txt holds a published solution (SINTEF
// format, "Route 1 : 5 3 7 ..."), the first attempt starts the LNS from it, with its fleet;
// the LNS only accepts improvements, so the baseline is never worse than it. Attempts with
// extra vehicles start from a random assignment. The converted start is kept as
// solomon_start.txt.

// Converts a SINTEF solution file to the solver's route_log format; returns the route count
// (0 if there is no file).
static int convertPublishedSolution(const string& sintefFile, const string& routeFile)
{
    std::ifstream in(sintefFile);
    if (!in.is_open()) return 0;
    std::ofstream out(routeFile);
    string line;
    int routes = 0;
    while (std::getline(in, line)) {
        if (line.rfind("Route", 0) != 0) continue;
        size_t colon = line.find(':');
        if (colon == string::npos) continue;
        std::stringstream ss(line.substr(colon + 1));
        out << routes << " :";
        int customer;
        while (ss >> customer) out << " " << customer;  // SINTEF customer k = solver index k (depot 0)
        out << "\n";
        routes++;
    }
    return routes;
}
exp0::exp0(const std::vector<std::string>& only, int iterations, bool completeGraph)
{
    const string layoutCity = "Cambridge";
    const int scale = 5;
    const double FLAT_TEMPERATURE_C = 20.0;  // the energy model's ideal: no temperature penalty, minimum HVAC
    const int MAX_EXTRA_VEHICLES = 5;

    string solutionsDir = string("solutions/") + (completeGraph ? "complete_" : "flatland_") + std::to_string(scale);
    string summaryFile = paths::resultsDir(solutionsDir) + "baseline_summary.csv";
    bool writeHeader = !std::filesystem::exists(summaryFile);
    std::ofstream summary(summaryFile, std::ios::app);
    if (writeHeader) {
        summary << "run_at,benchmark,vehicles,capacity,iterations,distance_m,late_stops,"
                   "tardiness_min,overload,last_return_min,seconds,clean,start";
        if (completeGraph) summary << ",mst_distance_m,mst_late_stops,mst_tardiness_min,mst_last_return_min,mst_min_soc";
        summary << "\n";
    }

    auto load = [&](const Benchmark& b, int vehicles) {
        Instance* instance = new Instance(paths::instanceFile(layoutCity, scale, b.name),
                                          paths::shortPathsFile(b.name),
                                          paths::weatherFile(layoutCity),
                                          b.capacity, vehicles,
                                          paths::edgeProfileFile(layoutCity, scale, b.name));
        instance->setVerbosity(0);
        instance->setDebug(0);
        instance->flattenWeather(FLAT_TEMPERATURE_C);
        instance->flattenCity();
        return instance;
    };

    auto lastReturnOf = [](Instance& instance) {
        int lastReturn = 0;
        for (const Route& r : instance.getRoutes()) lastReturn = std::max(lastReturn, r.getEndTime());
        return lastReturn;
    };

    for (const Benchmark& b : benchmarks()) {
        if (!only.empty() && std::find(only.begin(), only.end(), b.name) == only.end()) continue;

        if (!paths::inputsExist(layoutCity, scale, b.name)) {
            cerr << "skipping " << b.name << " (generate inputs with dataprep)" << endl;
            continue;
        }

        string solutionDir = paths::resultsDir(solutionsDir + "/" + b.name);
        string startFile = solutionDir + "solomon_start.txt";
        string sintefFile = (paths::resultsRoot() / "solomon_solutions" / (b.name + ".txt")).string();
        int publishedRoutes = convertPublishedSolution(sintefFile, startFile);
        int firstFleet = publishedRoutes > 0 ? publishedRoutes : b.vehicles;

        // Start from the published (or Solomon best-known) fleet and add vehicles until the
        // flatland solution is clean (no late stops, no overload). Matching Solomon's optimum
        // is not the aim; a clean baseline is, since exp1 measures what terrain and weather do to it.
        for (int vehicles = firstFleet; vehicles <= firstFleet + MAX_EXTRA_VEHICLES; vehicles++) {
            bool warm = publishedRoutes > 0 && vehicles == publishedRoutes;
            cout << "== " << b.name << (completeGraph ? " (complete graph)" : "") << ": " << vehicles
                 << " vehicles x " << b.capacity << ", " << iterations << " iterations, "
                 << (warm ? "published start" : "random start") << endl;

            Instance* instance = load(b, vehicles);
            if (completeGraph) instance->useCompleteGraph();
            if (warm) instance->readRoutesFromFile(startFile);
            instance->calculateRoutes();

            auto start = std::chrono::steady_clock::now();
            LNS optimizer(instance, solutionDir + "searchProgress.csv");
            SolutionScore score = optimizer.runLargeNeighbourSearch(iterations);
            double seconds = std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();

            instance->calculateRoutes();
            bool clean = instance->getTotalLate() == 0 && instance->getTotalOverload() == 0;
            bool last = vehicles == firstFleet + MAX_EXTRA_VEHICLES;

            std::time_t now = std::time(nullptr);
            char runAt[32];
            std::strftime(runAt, sizeof(runAt), "%Y-%m-%dT%H:%M:%S", std::localtime(&now));

            summary << runAt << "," << b.name << "," << vehicles << "," << b.capacity << ","
                    << iterations << "," << instance->getTotalDistance() << "," << instance->getTotalLate() << ","
                    << instance->getTotalTardiness() << "," << instance->getTotalOverload() << ","
                    << lastReturnOf(*instance) << "," << (int)seconds << "," << (clean ? 1 : 0) << "," << (warm ? "solomon" : "random");

            instance->writeRoutesToFile(solutionDir + "route_log.txt");

            if (completeGraph) {
                // the same routes, driven along the flat MST instead
                Instance* mst = load(b, vehicles);
                mst->readRoutesFromFile(solutionDir + "route_log.txt");
                mst->calculateRoutes();
                double minSoc = 1.0;
                for (const Route& r : mst->getRoutes())
                    for (const SoCEntry& e : r.getSoCDetail()) minSoc = std::min(minSoc, e.cumulative_soc);
                summary << "," << mst->getTotalDistance() << "," << mst->getTotalLate() << ","
                        << mst->getTotalTardiness() << "," << lastReturnOf(*mst) << "," << minSoc;
                delete mst;
            }
            summary << "\n";
            summary.flush();

            cout << b.name << " (" << vehicles << " vehicles): cost " << score.total_cost << ", distance "
                 << instance->getTotalDistance() << " m, late stops " << instance->getTotalLate()
                 << ", overload " << instance->getTotalOverload() << " (" << (int)seconds << " s)"
                 << (clean ? "" : " - not clean") << endl;

            if (clean || last) {
                if (!clean) cerr << b.name << ": still not clean with " << vehicles << " vehicles" << endl;
                instance->logDetailedRoutes(solutionDir + "detailed_route_log.csv");
                delete instance;
                break;
            }
            delete instance;
        }
    }
}

exp0::~exp0()
{
}
