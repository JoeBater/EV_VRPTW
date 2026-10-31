#ifndef MINUTERESULT_H
#define MINUTERESULT_H

using namespace std;

struct MinuteResult {
    double distance_accum;
    double soc_used_accum;
    double soc_regen_accum;
    double adjusted_speed;
};

#endif // MINUTERESULT