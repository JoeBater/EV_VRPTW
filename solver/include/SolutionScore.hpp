#ifndef SOLUTIONSCORE_H
#define SOLUTIONSCORE_H

using namespace std;

struct SolutionScore {
    double total_distance;
    int total_late;
    int total_overload;
    double total_cost;

    bool isBetterThan(const SolutionScore& other) const {
        if (total_late < other.total_late) return true;
        if (total_late == other.total_late && total_cost < other.total_cost) return true;
        return false;
    }
};
    
#endif // SOLUTIONSCORE_H