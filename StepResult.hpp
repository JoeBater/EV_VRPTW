#ifndef STEPRESULT_H
#define STEPRESULT_H

using namespace std;

struct StepResult {
    int distance;
    double soc_used;
    double soc_regen;
    int final_minute;
};

#endif // STEPRESULT