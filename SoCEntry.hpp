#ifndef SOCENTRY_H
#define SOCENTRY_H

using namespace std;

struct SoCEntry {
    int route_id;
    int minute;
    double soc_used;
    double soc_regen;
    double cumulative_soc;
    std::string activity;
    std::string timeliness;
    double speed;
    double dist_per_minute;
    int step_distance;
    double slope;
    double temperature;
    double precipitation;
    int loading;
    int leg_location_id;
    int step_location_id;
};

#endif // SOCENTRY_H
