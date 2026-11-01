#include "exp1.hpp"

#include "ProjectPaths.hpp"

static const int DAYS_IN_MONTH[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

exp1::exp1()
{
    string METHOD = "LNS";



    std::vector<std::string> benchmarks = {

        "r105",
//        "r101", "r104", "r204", "r205", "r208",
//        "c107", "c108", "c109",
//        "rc101", "rc104", "rc107", "rc201", "rc204", "rc207",
    };

    int scale = 5;

    for (size_t b = 0; b < benchmarks.size(); b++) {
        string benchmark = benchmarks[b];

        // Solutions are found on flattened terrain and weather, so the
        // optimisation city only supplies the site layout
        string solutionDir = paths::resultsDir("solutions/flatland_" + std::to_string(scale) + "/" + benchmark);

        vector<Route> solution;

        //string benchmark = "r101";
        for (int month = 1; month <= 1; month++) {

            string city = "Cambridge";

            if (!paths::inputsExist(city, scale, benchmark)) {
                cerr << "skipping " << benchmark << " (generate inputs with dataprep)" << endl;
                continue;
            }

            string instanceFile = paths::instanceFile(city, scale, benchmark);
            string treeFile = paths::shortPathsFile(benchmark);
            string weatherFile = paths::weatherFile(city);
            string searchProgressFile = paths::resultsDir("") + "searchProgress.csv";

            int NUM_VEHICLES = 17;
            int VEHICLE_CAPACITY = 200;
            //string solutionFile = "C:/Users/user/ML/Optimisation/VRPTW/solomon_solutions/Cambridge/r101/Cambridge_scaled_5_r101_elev.txt";

            Instance instance(instanceFile, treeFile, weatherFile, VEHICLE_CAPACITY, NUM_VEHICLES);
            cerr << "declared instance" << endl;
            SolutionScore finalCost;


            instance.setVerbosity(1);
            instance.setDebug(0);
            instance.displayInstanceData();
            instance.displayRoutes();

            //int month = 1;
            //int day = 1;
            //instance.getTodaysWeather(month,day);
            //instance.getMonthAverageWeather(month);
            instance.flattenWeather(20.0);
            instance.flattenCity();

            cerr << "set weather " << month << endl;
            instance.calculateRoutes();
            cerr << "done first route simulation" << endl;
            //instance.displayWeatherData();

            //instance.logDetailedRoutes("C:/Users/user/ML/Optimisation/VRPTW/detailed_route_log_"+std::to_string(month)+"_"+std::to_string(day)+".csv");

            //instance.readRoutesFromFile("C:/Users/user/ML/Optimisation/VRPTW/solomon/solomon_solutions/sintef_rc101.txt");


    //        if (METHOD == "AILS") {
    //            cout << "running AILS" << endl;
    //            int maxIterations = 50;
    //            int twoOptMaxNoImprovements = 10;
    //            int initialPerturbStrength = 5;
    //            AILS optimizer(&instance, twoOptMaxNoImprovements, maxIterations, initialPerturbStrength, searchProgressFile);
    //            finalCost = optimizer.runAdaptiveIteratedLocalSearch(LocalSearchMethod::FirstImproving, 5);
    //        }

            if (METHOD == "LNS") {
                cout << "running LNS" << endl;
                int maxIterations = 0.25e6;
                LNS optimizer(&instance, searchProgressFile);
                finalCost = optimizer.runLargeNeighbourSearch(maxIterations);
            }

            if (METHOD == "LNSar") {
                cout << "running LNS adaptive routing" << endl;
                int maxIterations = 0.25e6;
                LNSar optimizer(&instance, searchProgressFile);
                finalCost = optimizer.runAdaptiveRouteLargeNeighbourSearch(maxIterations);
            }

            //SolutionScore
            cout << finalCost.total_cost << endl;

            instance.displayRoutes();
            instance.writeRoutesToFile(solutionDir + "route_log_"+std::to_string(month)+".txt");
            instance.logDetailedRoutes(solutionDir + "detailed_route_log_"+std::to_string(month)+".csv");
            solution = instance.getRoutes();

            if (month == 1) {  // Jan - 31 days
                for (int day = 1; day <= 1; day++) {
                    //instance.getTodaysWeather(month, day);
                    instance.flattenWeather(20.0);
                    instance.calculateRoutes();
                    instance.logDetailedRoutes(solutionDir + "detailed_route_log_"+std::to_string(month)+"_"+std::to_string(day)+".csv");
                }
            }
        }

        if (solution.empty()) continue;

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
            //string solutionFile = "C:/Users/user/ML/Optimisation/VRPTW/solomon_solutions/Cambridge/r101/Cambridge_scaled_5_r101_elev.txt";

            Instance instance(cityFile, treeFile, weatherFile, VEHICLE_CAPACITY, NUM_VEHICLES);
            instance.setRoutes(solution);
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

exp1::~exp1()
{
}
