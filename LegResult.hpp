#ifndef LEGRESULT_H
#define LEGRESULT_H

using namespace std;

struct LegResult {
    int distance;
    double soc_used;
    double soc_regen;
    int final_minute;
    int step_count;
};

#endif // LEGRESULT