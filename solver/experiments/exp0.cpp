#include "exp0.hpp"

#include "ProjectPaths.hpp"

static const int DAYS_IN_MONTH[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

// Re-simulates saved solutions (from exp1) in every city, for every day of the year.
exp0::exp0()
{
    std::vector<std::string> benchmarks = {
        "c107", "c108", "c109",
        "r101", "r104", "r105", "r204", "r205", "r208",
        "rc101", "rc104", "rc107", "rc201", "rc204", "rc207",
    };

    int scale = 5;

    for (size_t b = 0; b < benchmarks.size(); b++) {
        string benchmark = benchmarks[b];

        //vector<Route> solution;

        string solutionDir = paths::resultsDir("solutions/flatland_" + std::to_string(scale) + "/" + benchmark);
        string solutionFile = solutionDir + "route_log_1.txt";
        if (!std::filesystem::exists(solutionFile)) {
            cerr << "skipping " << benchmark << ": no solution at " << solutionFile << " (run exp1 first)" << endl;
            continue;
        }

        // HORRIBLE FUNCTIONALITY TO LOG DATA RESULTS
        vector<string> cities;
        cities.push_back("Belfast");
        cities.push_back("Birmingham");
        cities.push_back("Bristol");
        cities.push_back("Cardiff");
        cities.push_back("Glasgow");
        cities.push_back("Leeds");
        cities.push_back("Liverpool");
        cities.push_back("London");
        cities.push_back("Plymouth");
        cities.push_back("Southampton");

//        cities.push_back("Aberwystwyth");
        cities.push_back("Bath");
        cities.push_back("Cambridge");
//        cities.push_back("Carlisle");
//        cities.push_back("Chichester");
//        cities.push_back("Derry");
//        cities.push_back("Inverness");
//        cities.push_back("Lancaster");
//        cities.push_back("Newcastle");
//        cities.push_back("Wrexham");

        for (size_t c = 0; c < cities.size(); c++) {

            string city = cities[c];

            if (!paths::inputsExist(city, scale, benchmark)) {
                cerr << "skipping " << city << " (generate inputs with dataprep)" << endl;
                continue;
            }

            string cityFile = paths::instanceFile(city, scale, benchmark);
            string treeFile = paths::shortPathsFile(benchmark);
            string weatherFile = paths::weatherFile(city);
            string cityDir = paths::resultsDir("solutions/flatland_" + std::to_string(scale) + "/" + benchmark + "/" + city);

            int NUM_VEHICLES = 20;
            int VEHICLE_CAPACITY = 200;

            Instance instance(cityFile, treeFile, weatherFile, VEHICLE_CAPACITY, NUM_VEHICLES);

            instance.readRoutesFromFile(solutionFile);
            //instance.setRoutes(solution);
            // evaluate against all cities
            for (int month = 1; month <= 12; month++) {
                for (int day = 1; day <= DAYS_IN_MONTH[month - 1]; day++) {
                    instance.getTodaysWeather(month, day);
                    instance.calculateRoutes();
                    instance.logDetailedRoutes(cityDir + "detailed_route_log_"+std::to_string(month)+"_"+std::to_string(day)+".csv");
                }
            }
        }
    }
}

exp0::~exp0()
{
}
