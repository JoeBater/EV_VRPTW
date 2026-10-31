#pragma once
#include <random>
#include <chrono>

#include "Instance.hpp"
#include "SolutionScore.hpp"
#include "StoreCount.hpp"
#include "StoreData.hpp"
#include "Quadrant.hpp"

using namespace std;


    
class LNSar
{
public:
    LNSar(Instance* instance,
        string plFile);
    
    ~LNSar();

    void adaptSearchParameters();
    std::vector<StoreCount> checkAllStoresVisitedOnce();
    void debugStoreCounts(const std::string& context);
    
    SolutionScore runAdaptiveRouteLargeNeighbourSearch(int iterations);
    
private:
    std::mt19937 rng;
    std::uniform_real_distribution<double> uniformDist;
    
    Instance* inst;
    int stagnationCounter;

    double destroyStrength = 0.2;
    std::vector<int> removed_stores;
    
    double w_distance = 1.0;
    double w_lateness = 1000.0;
    double w_capacity = 100.0;
    
    struct IterationLog {
        int iteration;
        double best;
        double cost;
        bool improved;
        string destroy_method;
        string repair_method;
        double w_distance;
        double w_lateness;
        double w_capacity;
    };
    

    
    Quadrant getQuadrant(int x, int y, int depotX, int depotY);
    
    std::vector<IterationLog> progressLog;
    string progressLogFile;
    
    int VERBOSE = 0;
    int DEBUG = 0;
    int MONITOR = 1;
    int VIDEO = 0;
    
    SolutionScore evaluateSolution();
    
    string destroy();
    string repair();
    
    void twoOptAllRoutes();
    
    int mm = 1;
    int dd = 1;
};

