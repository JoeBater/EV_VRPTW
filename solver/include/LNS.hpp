#pragma once
#include <random>
#include <string>
#include <vector>

#include "Instance.hpp"
#include "SolutionScore.hpp"

using namespace std;

// Adaptive large neighbourhood search (after Ropke & Pisinger 2006) over a fixed fleet: the
// instance's routes. A route may be emptied (the vehicle stays at the depot), so the fleet is an
// upper bound.
//
// Each iteration removes q stores (random, worst, related, a whole route, or late ones) and
// reinserts them (greedy or regret-2/3, with optional noise). Insertion positions are chosen with
// an estimate of the schedule: travel minutes from the network with no rain, slowed by the load
// as in Route. Only the routes that changed are then re-simulated in full, and that exact cost
// decides. Acceptance is simulated annealing; the best solution seen is kept and returned, so the
// result is never worse than the start. Operators are picked by adaptive weights. The best
// solution is finally polished route by route (2-opt and or-opt, exact cost).
//
// Cost = distance (m) + 10,000 per late stop + 100 per minute late + 10,000 per unit over capacity.
class LNS
{
public:
    // plFile: progress log, one row per iteration
    // videoDir: if non-empty, write per-iteration route snapshots to
    // <videoDir>/new/ and <videoDir>/best/ (for animation)
    LNS(Instance* instance, string plFile, string videoDir = "", unsigned seed = 1);
    ~LNS();

    SolutionScore runLargeNeighbourSearch(int iterations);

private:
    Instance* inst;
    std::mt19937 rng;
    string progressLogFile;
    string videoDir;
    int VERBOSE = 0;
    bool VIDEO = false;

    // problem data (site 0 is the depot)
    int nSites, nCustomers, capacity, horizon;
    vector<vector<int>> D;            // metres along the network
    vector<vector<double>> T0;        // minutes, no load, no rain, unrounded
    vector<int> ready, due, service, demand;
    double maxDistance = 1.0, maxDemand = 1.0;

    double w_distance = 1.0;
    double w_lateness = 10000.0;    // per late stop
    double w_tardiness = 100.0;     // per minute late, so the search can tell "nearly" from "very" late
    double w_capacity = 10000.0;    // per unit over capacity

    // exact cost of each route (from the full simulation)
    struct RouteCost {
        int distance = 0, late = 0, tardiness = 0, overload = 0;
        double cost = 0.0;
    };
    vector<RouteCost> cost;
    double currentCost = 0.0;

    // estimated schedule of each route, for choosing insertions
    struct Schedule {
        vector<int> arrive;   // per stop, then the depot
        vector<int> depart;   // per stop
        vector<int> latest;   // latest arrival that keeps the rest of the route on time; then the depot
        vector<int> legLoad;  // load on the leg into each stop, then the depot
        int load = 0;
    };
    vector<Schedule> sched;

    // routes changed this iteration, with what to restore if it is rejected
    struct Backup {
        int r;
        Route route;
        RouteCost cost;
        Schedule schedule;
    };
    vector<Backup> backups;
    vector<char> touched;
    vector<int> removed;
    vector<int> routeOf, posOf;   // where each store is, refreshed before each destroy

    vector<vector<int>> best;
    double bestCost = 0.0;

    struct Operator {
        string name;
        double weight = 1.0, score = 0.0;
        int uses = 0;
    };
    vector<Operator> destroyOps, repairOps;

    vector<int>& seq(int r) { return inst->getRoutes()[r].getRoute(); }
    int legTime(int a, int b, int load) const;
    RouteCost evaluate(int r);
    void buildSchedule(int r);
    void touch(int r);
    void locate();

    int pick(const vector<Operator>& ops);
    void destroy(int op, int q);
    void removeMarked(const vector<char>& mark);
    void randomRemoval(int q, vector<char>& mark);
    void worstRemoval(int q, vector<char>& mark);
    void relatedRemoval(int q, vector<char>& mark, vector<int> seeds);
    void routeRemoval(vector<char>& mark);
    void lateRemoval(int q, vector<char>& mark);

    double insertionCost(int c, int r, int p);
    void repair(int k, bool noise);

    double estimateCost(const vector<int>& s) const;
    bool tryRoute(int r, const vector<int>& candidate);
    void polishRoute(int r);

    void saveBest();
    void restoreBest();
    void writeRoutes(const string& filename, const vector<vector<int>>& routes) const;
};
