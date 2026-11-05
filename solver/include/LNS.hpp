#pragma once
#include <random>
#include <chrono>

#include "Instance.hpp"
#include "SolutionScore.hpp"
#include "StoreCount.hpp"
#include "StoreData.hpp"
#include "Quadrant.hpp"

using namespace std;

//enum Quadrant {
//    NE = 0,
//    NW = 1,
//    SW = 2,
//    SE = 3
//};
    
class LNS
{
public:
    // videoDir: if non-empty, write per-iteration route snapshots to
    // <videoDir>/new/ and <videoDir>/best/ (for animation)
    LNS(Instance* instance,
        string plFile,
        string videoDir = "");
    
    ~LNS();

    void adaptSearchParameters();
    std::vector<StoreCount> checkAllStoresVisitedOnce();
    void debugStoreCounts(const std::string& context);
    
    SolutionScore runLargeNeighbourSearch(int iterations);
    
private:
    std::mt19937 rng;
    std::uniform_real_distribution<double> uniformDist;
    
    Instance* inst;
    int stagnationCounter;

    double destroyStrength = 0.2;
    std::vector<int> removed_stores;
    
    // distance is in metres, so a late stop must cost far more than a detour
    double w_distance = 1.0;
    double w_lateness = 10000.0;    // per late stop
    double w_tardiness = 100.0;     // per minute late, so the search can tell "nearly" from "very" late
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
    string videoDir;
    
    int VERBOSE = 0;
    int DEBUG = 0;
    int MONITOR = 1;
    int VIDEO = 0;   // enabled by passing videoDir
    
    SolutionScore evaluateSolution();
    
    string destroy();
    string repair();
    
    void twoOptAllRoutes();
    
    int mm = 1;
    int dd = 1;
};

