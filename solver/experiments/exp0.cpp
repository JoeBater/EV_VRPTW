#include "exp0.hpp"



exp0::exp0()
{
    std::vector<std::string> benchmarks = {
        "c107", "c108", "c109", 
        "r101", "r104", "r105", "r204", "r205", "r208", 
        "rc101", "rc104", "rc107", "rc201", "rc204", "rc207", 
    };

    int scale = 10;

    for (size_t b = 0; b < benchmarks.size(); b++) {
        string benchmark = benchmarks[b];
    
        //vector<Route> solution;
        
        
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
//        cities.push_back("Bath");
//        cities.push_back("Cambridge");
//        cities.push_back("Carlisle");
//        cities.push_back("Chichester");
//        cities.push_back("Derry");
//        cities.push_back("Inverness");
//        cities.push_back("Lancaster");
//        cities.push_back("Newcastle");
//        cities.push_back("Wrexham");
        
        for (size_t c = 0; c < cities.size(); c++) {
            
            string city = cities[c];
            
            
            string cityFile = "C:/Users/user/ML/Optimisation/VRPTW/solomon/checked/"+city+"_"+std::to_string(scale)+"_" + benchmark +".csv";
            string treeFile = "C:/Users/user/ML/Optimisation/VRPTW/solomon/short_paths/" + benchmark +"_short_paths.txt";
            string weatherFile = "C:/Users/user/ML/Optimisation/VRPTW/weather_data/open-meteo/"+city+"-weather.csv";
            //string searchProgressFile = "C:/Users/user/ML/Optimisation/VRPTW/searchProgress.csv";
            
            int NUM_VEHICLES = 20;
            int VEHICLE_CAPACITY = 200;
            //string solutionFile = ;
            
            Instance instance(cityFile, treeFile, weatherFile, VEHICLE_CAPACITY, NUM_VEHICLES);
            
            instance.readRoutesFromFile("C:/Users/user/ML/Optimisation/VRPTW/solomon/solomon_solutions/flatland_"+std::to_string(scale)+"/"+benchmark+"/route_log_1.txt");
            //instance.setRoutes(solution);
            // evaluate against all cities
            for (int month = 1; month <= 12; month++) {

                
                if (month == 1) {  // Jan - 31 days
                    for (int day = 1; day <= 31; day++) {
                        instance.getTodaysWeather(month, day);
                        instance.calculateRoutes();
                        instance.logDetailedRoutes("C:/Users/user/ML/Optimisation/VRPTW/solomon/solomon_solutions/flatland_"+std::to_string(scale)+"/"+benchmark+"/"+city+"/detailed_route_log_"+std::to_string(month)+"_"+std::to_string(day)+".csv");
                    }
                }
                if (month == 2) {   // Feb
                    for (int day = 1; day <= 28; day++) {
                        instance.getTodaysWeather(month, day);
                        instance.calculateRoutes();
                        instance.logDetailedRoutes("C:/Users/user/ML/Optimisation/VRPTW/solomon/solomon_solutions/flatland_"+std::to_string(scale)+"/"+benchmark+"/"+city+"/detailed_route_log_"+std::to_string(month)+"_"+std::to_string(day)+".csv");
                    }
                }
                if (month == 3) {   // Mar
                    for (int day = 1; day <= 31; day++) {
                        instance.getTodaysWeather(month, day);
                        instance.calculateRoutes();
                        instance.logDetailedRoutes("C:/Users/user/ML/Optimisation/VRPTW/solomon/solomon_solutions/flatland_"+std::to_string(scale)+"/"+benchmark+"/"+city+"/detailed_route_log_"+std::to_string(month)+"_"+std::to_string(day)+".csv");
                    }
                }
                if (month == 4) {   // Apr
                    for (int day = 1; day <= 30; day++) {
                        instance.getTodaysWeather(month, day);
                        instance.calculateRoutes();
                        instance.logDetailedRoutes("C:/Users/user/ML/Optimisation/VRPTW/solomon/solomon_solutions/flatland_"+std::to_string(scale)+"/"+benchmark+"/"+city+"/detailed_route_log_"+std::to_string(month)+"_"+std::to_string(day)+".csv");
                    }
                }
                if (month == 5) {   // May
                    for (int day = 1; day <= 31; day++) {
                        instance.getTodaysWeather(month, day);
                        instance.calculateRoutes();
                        instance.logDetailedRoutes("C:/Users/user/ML/Optimisation/VRPTW/solomon/solomon_solutions/flatland_"+std::to_string(scale)+"/"+benchmark+"/"+city+"/detailed_route_log_"+std::to_string(month)+"_"+std::to_string(day)+".csv");
                    }
                }
                if (month == 6) {   // Jun
                    for (int day = 1; day <= 30; day++) {
                        instance.getTodaysWeather(month, day);
                        instance.calculateRoutes();
                        instance.logDetailedRoutes("C:/Users/user/ML/Optimisation/VRPTW/solomon/solomon_solutions/flatland_"+std::to_string(scale)+"/"+benchmark+"/"+city+"/detailed_route_log_"+std::to_string(month)+"_"+std::to_string(day)+".csv");
                    }
                }
                if (month == 7) {   // Jul
                    for (int day = 1; day <= 31; day++) {
                        instance.getTodaysWeather(month, day);
                        instance.calculateRoutes();
                        instance.logDetailedRoutes("C:/Users/user/ML/Optimisation/VRPTW/solomon/solomon_solutions/flatland_"+std::to_string(scale)+"/"+benchmark+"/"+city+"/detailed_route_log_"+std::to_string(month)+"_"+std::to_string(day)+".csv");
                    }
                }
                if (month == 8) {   // Aug
                    for (int day = 1; day <= 31; day++) {
                        instance.getTodaysWeather(month, day);
                        instance.calculateRoutes();
                        instance.logDetailedRoutes("C:/Users/user/ML/Optimisation/VRPTW/solomon/solomon_solutions/flatland_"+std::to_string(scale)+"/"+benchmark+"/"+city+"/detailed_route_log_"+std::to_string(month)+"_"+std::to_string(day)+".csv");
                    }
                }
                if (month == 9) {   // Sep
                    for (int day = 1; day <= 30; day++) {
                        instance.getTodaysWeather(month, day);
                        instance.calculateRoutes();
                        instance.logDetailedRoutes("C:/Users/user/ML/Optimisation/VRPTW/solomon/solomon_solutions/flatland_"+std::to_string(scale)+"/"+benchmark+"/"+city+"/detailed_route_log_"+std::to_string(month)+"_"+std::to_string(day)+".csv");
                    }
                }
                if (month == 10) {  // Oct
                    for (int day = 1; day <= 31; day++) {
                        instance.getTodaysWeather(month, day);
                        instance.calculateRoutes();
                        instance.logDetailedRoutes("C:/Users/user/ML/Optimisation/VRPTW/solomon/solomon_solutions/flatland_"+std::to_string(scale)+"/"+benchmark+"/"+city+"/detailed_route_log_"+std::to_string(month)+"_"+std::to_string(day)+".csv");
                    }
                }
                if (month == 11) {  // Nov
                    for (int day = 1; day <= 30; day++) {
                        instance.getTodaysWeather(month, day);
                        instance.calculateRoutes();
                        instance.logDetailedRoutes("C:/Users/user/ML/Optimisation/VRPTW/solomon/solomon_solutions/flatland_"+std::to_string(scale)+"/"+benchmark+"/"+city+"/detailed_route_log_"+std::to_string(month)+"_"+std::to_string(day)+".csv");
                    }
                }
                if (month == 12) {  // Dec
                    for (int day = 1; day <= 31; day++) {
                        instance.getTodaysWeather(month, day);
                        instance.calculateRoutes();
                        instance.logDetailedRoutes("C:/Users/user/ML/Optimisation/VRPTW/solomon/solomon_solutions/flatland_"+std::to_string(scale)+"/"+benchmark+"/"+city+"/detailed_route_log_"+std::to_string(month)+"_"+std::to_string(day)+".csv");
                    }
                }
            
            }
        }
    }
}

exp0::~exp0()
{
}

