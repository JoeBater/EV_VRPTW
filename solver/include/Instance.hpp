#pragma once

#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <iterator>
#include <iostream>
#include <cmath>
#include <cstdlib>

#include "StoreData.hpp"
#include "Route.hpp"
#include "MST.hpp"

using namespace std;

class Instance
{
public:
    Instance(string instanceFile, string spanningTreeFile, string weatherFile, int vc, int nr);
    ~Instance();

    void readInstanceFile(string filename);
    
    int LATE_PENALTY = 1000;
    void setLatePenalty(int p) { LATE_PENALTY = p; }
    
    
    
    void setVerbosity(int v);
    int getVerbosity();
    void setDebug(int d);
    int getDebug();
    
    int getInstanceDataSize();
    std::vector<StoreData> getInstanceData() const;
    void displayInstanceData();
    void displayDistanceMatrix();
    vector<vector<int>> getDistanceMatrix();
    const vector<vector<int>>& getDistanceMatrix() const;
    int getDistance(int s1, int s2);
    
    vector<vector<double>> getSlopeMatrix();
    const vector<vector<double>>& getSlopeMatrix() const;
    void flattenCity();
    double getElevation(int s1);
    double getSlope(int s1, int s2);
    
    MST getSpanningTree();
    const MST getSpanningTree() const;
    
    bool isTimeWindowFeasible(const std::vector<int>& rte) const;
    
    vector<int> getReadyTimes();
    const vector<int>& getReadyTimes() const;
    vector<int> getDueTimes();
    const vector<int>& getDueTimes() const;
    vector<int> getServiceTimes();
    const vector<int>& getServiceTimes() const;
    int getServiceTimeSite(int s);
    vector<int> getDemands();
    const vector<int>& getDemands() const;
    const vector<double>& getTemperatures() const;
    const vector<double>& getRainfall() const;
    int getVehicleCapacity();
    const int& getVehicleCapacity() const;
    
    void readSpanningTree(string filename);
    void readWeatherFile(string filename);
    void displayWeatherData();
    
    struct WeatherData {
                int month;
                int day;
                int hour;
                double temp_C;  // in Celsius
                double rain_mm;
            };
    void getTodaysWeather(int mm, int dd);
    void getMonthAverageWeather(int mm);
    void flattenWeather(double tt);

    void setVehicleCapacity(int c);
    int estimateNumRoutes();
    void initializeRoutes(int nr); // ...
    void initializeRoutesCapacityConstrained(int nr); // ...
    void heuristicallySeedRoutes(int nr);
    void setRoutes(std::vector<Route> rtes); 
    void displayRoutes();
    int getTotalDistance();
    int getTotalLate();
    const std::vector<int>& getLateFlags(int routeIndex) const {
        return routes[routeIndex].getLateFlags();
    }

    const std::vector<int>& getLatenessPerStop(int routeIndex) const {
        return routes[routeIndex].getLatenessPerStop();
    }
    const std::vector<int>& getEarlinessPerStop(int routeIndex) const {
        return routes[routeIndex].getEarlinessPerStop();
    }
    int getTotalOverload();
    
    void readRoutesFromFile(string filename);
    void writeRoutesToFile(string filename);
        
    void logDetailedRoutes(string filename);
    
    vector<Route>& getRoutes();
    const vector<Route>& getRoutes() const;
    
    bool checkAllStoresVisitedOnce();
    void calculateRoutes();
    
private:
    
    int VERBOSE = 1;
    int DEBUG = 1;
    int LOGGING = 1;
    
    vector<StoreData> instanceData;
    MST spanning_tree;
    vector<vector<int>> distanceMatrix; // distance matrix between customers
    vector<vector<double>> slopeMatrix; // distance matrix between customers
    void generateDistanceMatrix();
    void generateSlopeMatrix();
    
    int vehicle_capacity;
    
    vector<int> demands; // vector of demands for each customer
    void setDemands();   // function to initialise demands vector

    vector<int> ready_times; // vector of ready times 
    void setReadyTimes();    // ...
    
    vector<int> due_times; // vector of due times
    void setDueTimes();    // ... 

    vector<int> service_times; // vector of service times
    void setServiceTimes();    // ...

    vector<int> late_penalties;
    void setLatePenalties();
    vector<int> getLatePenalties();
    
    vector<Route> routes;    // vector of routes for each vehicle
    //bool checkAllStoresVisitedOnce();
    int improvements = 0;
    
    double extractScaleNumber(const std::string& filename);
            
    vector<WeatherData> weatherData; // weather data for the city
    vector<double> temperatures;
    vector<double> rainfall;
    
    int adaptiveScoreTrend(vector<int> scores);
    
    const int twoOptSearchLimit = 200;
};


