#include "Route.hpp"

Route::Route() {
    r.clear(); // default constructor - empty Route
};

Route::~Route() {
    r.clear();
}

Route::Route(vector<int> x) : r(x) {};

Route::Route(const Route& other)
    : route_number(other.route_number),
      r(other.r),
      is_feasible(other.is_feasible),
      distances(other.distances),
      slopes(other.slopes),
      speeds(other.speeds),
      total_distance(other.total_distance),
      arrival_times(other.arrival_times),
      depot_departure_time(other.depot_departure_time),
      departure_times(other.departure_times),
      late_flags(other.late_flags),
      total_late(other.total_late),
      loadings(other.loadings),
      max_loading(other.max_loading),
      soc_summary(other.soc_summary),
      soc_detail(other.soc_detail),
      CAPACITY(other.CAPACITY),
      depart_depot_time(other.depart_depot_time),
      return_to_depot_time(other.return_to_depot_time) {} // copy constructor

Route& Route::operator=(const Route& other) {
    if (this == &other) return *this;

    route_number = other.route_number;
    r = other.r;
    is_feasible = other.is_feasible;
    distances = other.distances;
    slopes = other.slopes;
    speeds = other.speeds;
    total_distance = other.total_distance;
    arrival_times = other.arrival_times;
    depot_departure_time = other.depot_departure_time;
    departure_times = other.departure_times;
    late_flags = other.late_flags;
    total_late = other.total_late;
    loadings = other.loadings;
    max_loading = other.max_loading;
    soc_summary = other.soc_summary;
    soc_detail = other.soc_detail;
    CAPACITY = other.CAPACITY;
    depart_depot_time = other.depart_depot_time;
    return_to_depot_time = other.return_to_depot_time;

    return *this;
}

void Route::setRouteNumber(int nr) {
    route_number = nr;
}

// set the Route
void Route::setRoute(const vector<int>& new_Route) {
    r = new_Route;
}

int Route::getRoute(int s) const { 
    if (s < 0 or s >= (int)r.size()) {
        cerr << "accessing store out of route bounds" << endl;
        exit(-1);
    }
    return r[s]; 
}

int& Route::getRoute(int s) { 
    if (s < 0 or s >= (int)r.size()) {
        cerr << "accessing store out of route bounds" << endl;
        exit(-1);
    }
    return r[s]; 
}

vector<int> Route::getRoute() const { 
    return r; 
}

vector<int>& Route::getRoute() { 
    return r; 
}

std::pair<double, double> Route::speedAndFactor(double distance) {
    double speed;
    double factor;

    if (distance < 2000) {
        speed = 25.0;
        factor = 0.9;
    } else if (distance < 5000) {
        speed = 35.0;
        factor = 1.0;
    } else {
        speed = 55.0;
        factor = 1.1;
    }

    return {speed, factor};
}

double Route::getDistance() {
    return distances[distances.size()-1];
}

double Route::slopeFactor(double slope) {
    if (slope > 0.1) return 1.3;      // Steep uphill
    else if (slope >= 0.075) return 1.25; // Mild uphill
    else if (slope >= 0.05) return 1.17;  // noticeable uphill
    else if (slope >= 0.03) return 1.1;  // moderate uphill
    else if (slope >= 0.015) return 1.07; // mild
    else return 1.0;
}

double Route::regenBrakingGain(double slope, double speed, double load_factor, double rainfall) {
    if (!REGEN_BRAKING) return 0.0;

    // No regen if speed is too low or slope is flat
    if (slope >= -0.02 || speed < 5.0) return 0.0;

    // Rainfall penalty: wet roads reduce regen efficiency
    double weather_penalty = (rainfall > 2.0) ? 0.5 : 1.0;

    // Base regen model: steeper slope and heavier load = more gain
    double regen_base = BASE_REGEN * std::abs(slope) * speed * load_factor;

    return regen_base * weather_penalty;
}

double Route::temperatureFactor(double temp) {
    if (temp < 0) return 1.25;       // 25% more energy needed
    else if (temp < 10) return 1.15;
    else if (temp <= 25) return 1.0;
    else if (temp <= 35) return 1.05;
    else return 1.10;
}

double Route::loadFactor(int load) {
    double alpha = 0.175;
    return 1.0 + alpha * (load / (double)(CAPACITY));
}

void Route::simulateRouteTimelineMST(const vector<vector<int>>& distanceMatrix, 
    const vector<vector<double>>& slopeMatrix, 
    const MST& spanning_tree,
    const vector<int>& readyTimes, 
    const vector<int>& dueTimes, 
    const vector<int>& serviceTimes, 
    const int& capacity, 
    const vector<int>& demands,
    const vector<double>& temperatures,
    const vector<double>& rainfall) {
        
    // guard against r being empty - and then bail
    if (r.empty()) {
        cerr << "route is empty" << endl;
        total_distance = 0;
        distances.clear();
        slopes.clear();
        speeds.clear();
        arrival_times.clear();
        departure_times.clear();
        late_flags.clear();
        loadings.clear();
        total_late = 0;
        max_loading = 0;
        return;
    }
    
    // *** now for the heavy lifting! ***
    
    // distances
    distances = std::vector<int>(r.size() + 1, 0);  // +1 for depot return

    // arrival times & departure times
    arrival_times = std::vector<int>(r.size() + 1, 0);
    departure_times = std::vector<int>(r.size() + 1, 0);
    
    late_flags = std::vector<int>(r.size()+1, 0);
    lateness_per_stop = std::vector<int>(r.size()+1, 0);
    earliness_per_stop = std::vector<int>(r.size()+1, 0);
    
    // SoC
    double cumulative_soc = 1.0;  // initial SOC
    soc_summary = std::vector<double>(r.size() + 1, 0.0);
    soc_detail.clear();
    
    // calculate the loads - load back to depot is immaterial - it should be zero
    loadings = std::vector<int>(r.size(), 0);
    int total_load = 0;
    for (size_t i = 0; i < r.size(); i++) {
        total_load += demands[r[i]];
    }
    loadings[0] = total_load;
    for (size_t i = 1; i < r.size(); i++) {
        loadings[i] = loadings[i-1]-demands[r[i]];
    }
    loadings.push_back(0); // back to depot, empty
    
    
    is_feasible = (loadings[0] <= capacity);
    
    auto simulateStep = [&](int step_from, int step_to, int leg_to, int load, int& current_minute, bool apply_impulse) -> StepResult 
    {
        double step_soc_used = 0.0;
        double step_soc_regenerated = 0.0;
        int step_dist = distanceMatrix[step_from][step_to];
        double step_slope = slopeMatrix[step_from][step_to];
        double load_factor = loadFactor(load);
        auto [notional_speed, _] = speedAndFactor(step_dist);
        
        while (step_dist > 0.0) {
            
            auto [hr, _] = getSimulationTimeHoursMins(current_minute);
            
            double temp_factor = temperatureFactor(temperatures[hr]);
            double speed_mod = speedModifier(step_slope, rainfall[hr], load);
            double distance_minute = ((notional_speed * speed_mod) / 60.0) * 1000.0;
            double speed = distance_minute * 60.0 / 1000.0;
            double speed_factor = speedFactor(speed);
            
            // LATER - we want to change this to bring regen braking gains out explicitly 
            double minute_slope_factor = slopeFactor(step_slope);
            if (step_slope < -0.01 && rainfall[hr] > 2.0)
                minute_slope_factor = 1.0;
            
            double soc_used = BASE_SOC * distance_minute * temp_factor * minute_slope_factor * speed_factor * load_factor;
            if (apply_impulse) {
                soc_used += computeAccelerationImpulse(load, speed);
                apply_impulse = false;
            }
            double soc_regen = regenBrakingGain(step_slope, speed, load_factor, rainfall[hr]);
            soc_used -= soc_regen;
            step_soc_regenerated += soc_regen;

            cumulative_soc -= soc_used;
            step_soc_used += soc_used;

            SoCEntry soc {
                route_number, current_minute, soc_used, soc_regen, cumulative_soc, "driving", "-",
                speed, distance_minute, distanceMatrix[step_from][step_to], step_slope, temperatures[hr], rainfall[hr], load,
                leg_to, step_to
            };
            soc_detail.push_back(soc);
        
            step_dist -= distance_minute;
            current_minute++;
        }

        StepResult result;
        result.soc_used = step_soc_used;
        result.soc_regen = step_soc_regenerated;
        result.distance = total_distance;
        result.final_minute = current_minute;
        result.distance = distanceMatrix[step_from][step_to];
        
        return result;
    };
            
    auto simulateLeg = [&](int from, int to, int load, int& current_minute, bool apply_impulse) -> LegResult 
    {
        vector<int> steps = spanning_tree.getSteps(from, to);
        
        LegResult leg_result;
        leg_result.distance = 0;
        leg_result.soc_used = 0.0;
        leg_result.soc_regen = 0.0;
        leg_result.final_minute = current_minute;
        leg_result.step_count = 0;
        
        if (steps.size() == 0) {
            StepResult step_result = simulateStep(from, to, to, load, current_minute, apply_impulse);
            leg_result.distance += step_result.distance;
            leg_result.soc_used += step_result.soc_used - step_result.soc_regen;
            leg_result.soc_regen += step_result.soc_regen;
            leg_result.final_minute = step_result.final_minute;
            
        } else {
            
            StepResult step_result = simulateStep(from, steps[0], to, load, current_minute, apply_impulse);
            leg_result.distance += step_result.distance;
            leg_result.soc_used += step_result.soc_used - step_result.soc_regen;
            leg_result.soc_regen += step_result.soc_regen;
            
            for (size_t s = 0; s < steps.size()-1; s++) {
                step_result = simulateStep(steps[s], steps[s+1], to, load, current_minute, apply_impulse);
                leg_result.distance += step_result.distance;
                leg_result.soc_used += step_result.soc_used - step_result.soc_regen;
                leg_result.soc_regen += step_result.soc_regen;
                leg_result.step_count += 1;
            }
            
            step_result = simulateStep(steps[steps.size()-1], to, to, load, current_minute, apply_impulse);
            leg_result.distance += step_result.distance;
            leg_result.soc_used += step_result.soc_used - step_result.soc_regen;
            leg_result.soc_regen += step_result.soc_regen;
            leg_result.final_minute = step_result.final_minute;
            leg_result.step_count += 1;
        }

        
        return leg_result;
    };

    auto simulateService = [&](int site_id, int site_idx, int load, int& current_minute, double& parasitic_soc_used) {
        auto [hr, _] = getSimulationTimeHoursMins(current_minute);
        int extended_service = adjustedServiceTime(serviceTimes[site_id], rainfall[hr]);
        
        int service_start_time = std::max(arrival_times[site_idx], readyTimes[site_id]);
        departure_times[site_idx] = service_start_time + extended_service;
    
        double soc_regen = 0.0;
        
        while (current_minute < departure_times[site_idx]) {
            
            auto [hr, _] = getSimulationTimeHoursMins(current_minute);
            //cerr << "current_minute " << current_minute << " hr " << hr << " site_id " << site_id << " arrival time " << arrival_times[site_idx] << " service time " << extended_service << endl;
            double soc_used = hvacDrawKW(temperatures[hr]) / 60.0 / FULL_BATTERY_KWH;
            cumulative_soc -= soc_used;
            parasitic_soc_used += soc_used;

            
            std::string activity = (current_minute < readyTimes[site_id]) ? "waiting" : "at_site";
            std::string timeliness = "";
            if (current_minute < readyTimes[site_id])
                timeliness = "early";
            else if (current_minute > dueTimes[site_id])
                timeliness = "late";
            else
                timeliness = "on_time";
                
            
            
            SoCEntry soc {
                route_number, current_minute, soc_used, soc_regen, cumulative_soc, activity, timeliness,
                0.0, 0.0, 0, 0.0, temperatures[hr], rainfall[hr], load,
                site_id, site_id
            };
            soc_detail.push_back(soc);
            

            current_minute++;
        }
    };

    
    auto simulateRoute = [&]() {
        
        int m = depot_departure_time;
        cumulative_soc = 1.0;
        soc_detail.clear();
        
        // pre-load the soc-detail in case of any delayed departures
        for (int mm = 0; mm < depot_departure_time; mm++) {
            auto [hr, _] = getSimulationTimeHoursMins(mm);
            SoCEntry soc {
                route_number, mm, 0.0, 0.0, 1.0, "delayed-start", "delayed-start",
                0.0, 0.0, 0, 0.0, temperatures[hr], rainfall[hr], loadings[0],
                0, 0
            };
            soc_detail.push_back(soc);
        }

        LegResult leg_result = simulateLeg(0, r[0], loadings[0], m, USE_ACCELERATION_IMPULSE);
        arrival_times[0] = m;
        distances[0] = leg_result.distance;

        double parasitic_soc_used = 0.0;
        simulateService(r[0], 0, loadings[0], m, parasitic_soc_used);
        soc_summary[0] = 1.0 - leg_result.soc_used - parasitic_soc_used;

        for (size_t i = 1; i < r.size(); ++i) {
            int from = r[i-1];
            int to = r[i];
            departure_times[i-1] = m;
            leg_result = simulateLeg(from, to, loadings[i], m, USE_ACCELERATION_IMPULSE && m == departure_times[i-1]);
            distances[i] = distances[i-1] + leg_result.distance;
            arrival_times[i] = m;
            parasitic_soc_used = 0.0;
            simulateService(to, i, loadings[i], m, parasitic_soc_used);
            soc_summary[i] = soc_summary[i-1] - leg_result.soc_used - parasitic_soc_used;
        }

        int last_site = r.back();
        departure_times[r.size() - 1] = m;
        leg_result = simulateLeg(last_site, 0, loadings.back(), m, USE_ACCELERATION_IMPULSE && m == departure_times.back());
        int final_idx = r.size();
        distances[final_idx] = distances[final_idx-1] + leg_result.distance;
        arrival_times[final_idx] = leg_result.final_minute;
        soc_summary[final_idx] = soc_summary[final_idx - 1] - leg_result.soc_used;

        // lateness checks
        total_late = 0;
        is_feasible = 1;
        for (size_t i = 0; i < r.size(); i++) {
            if (arrival_times[i] > dueTimes[r[i]]) {
                late_flags[i] = 1;
                lateness_per_stop[i] = arrival_times[i] - dueTimes[r[i]];
                total_late += 1;
                is_feasible = 0;
            }
            if (arrival_times[i] < readyTimes[r[i]]) {
                earliness_per_stop[i] = readyTimes[r[i]] - arrival_times[i];
            }
        }
        if (arrival_times[r.size()] > dueTimes[0]) {
            late_flags[r.size()] = 1;
            lateness_per_stop[r.size()] = arrival_times[r.size()] - dueTimes[0];
            total_late += 1;
            is_feasible = 0;
        }
    };

/*
    // from the depot to the first store
    int m = depot_departure_time;
    
    LegResult leg_result = simulateLeg(0, r[0], loadings[0], m, USE_ACCELERATION_IMPULSE);
    arrival_times[0] = m;
    distances[0] = leg_result.distance;
    
    double parasitic_soc_used = 0.0;
    simulateService(r[0], 0, loadings[0], m, parasitic_soc_used);
    soc_summary[0] = 1.0 - leg_result.soc_used - parasitic_soc_used;
    
    // all intermediary stores
    for (size_t i = 1; i < r.size(); ++i) {
        int from = r[i-1];
        int to = r[i];
        departure_times[i-1] = m;
        leg_result = simulateLeg(from, to, loadings[i], m, USE_ACCELERATION_IMPULSE && m == departure_times[i-1]);
        distances[i] = distances[i-1] + leg_result.distance;
        arrival_times[i] = m;
        parasitic_soc_used = 0.0;
        simulateService(to, i, loadings[i], m, parasitic_soc_used);
        soc_summary[i] = soc_summary[i-1] - leg_result.soc_used - parasitic_soc_used;
    }

    // back to depot from last store
    int last_site = r.back();
    
    departure_times[r.size() - 1] = m;
    leg_result = simulateLeg(last_site, 0, loadings.back(), m, USE_ACCELERATION_IMPULSE && m == departure_times.back());
    int final_idx = r.size();
    distances[final_idx] = distances[final_idx-1] + leg_result.distance;
    arrival_times[final_idx] = leg_result.final_minute;
    soc_summary[final_idx] = soc_summary[final_idx - 1] - leg_result.soc_used;
        
    // check the arrival times ### WRONG - can arrive on time but still end up late!
    late_flags = std::vector<int>(r.size()+1, 0);
    lateness_per_stop = std::vector<int>(r.size()+1, 0);
    earliness_per_stop = std::vector<int>(r.size(), 0);
    total_late = 0;
    is_feasible = 1;
    for (size_t i = 0; i < r.size(); i++) {         // all legs on the route
        if (arrival_times[i] > dueTimes[r[i]]) {
            late_flags[i] = 1;
            lateness_per_stop[i] = arrival_times[i] - dueTimes[r[i]];
            total_late += 1;
            is_feasible = 0;
        }
        if (arrival_times[i] < readyTimes[r[i]]) {
            earliness_per_stop[i] = readyTimes[r[i]] - arrival_times[i];
        }
    }
    if (arrival_times[r.size()] > dueTimes[0]) {    // and the depot
        late_flags[r.size()] = 1;
        lateness_per_stop[r.size()] = arrival_times[r.size()] - dueTimes[0];
        total_late += 1;
        is_feasible = 0;
    }
     */


    depot_departure_time = 0;
    simulateRoute();
    
    if (OPTIMISE_DEPOT_DEPARTURE == true) {
        
        depot_departure_time = 0; // ensure any changes reset
        int direction = 0; // +1 = delaying, -1 = advancing, 0 = initial
        const int tolerance = 5; // minutes
        const int max_iterations = 10;

        for (int iteration = 0; iteration < max_iterations; ++iteration) {
            simulateRoute(); // updates arrival_times[0], earliness_per_stop[0]

            int earliness = earliness_per_stop[0];
            int delta = earliness - tolerance;

            if (delta <= 0) {
                // Within tolerance: no unnecessary early arrival
                break;
            }

            int new_direction = (delta > 0) ? +1 : -1;
            if (direction != 0 && new_direction != direction) {
                // Oscillation detected
                break;
            }
            direction = new_direction;

            depot_departure_time += delta;
            //cerr << "resetting depot departure time " << depot_departure_time << endl;
            
            // guard to ensure that we don't try to leave before midnight
            if (depot_departure_time < 0) {
                depot_departure_time = 0;
                break;
            }
        }
    }
}

/*
// function to calculate the distance of a Route - note begins and ends at the depot
void Route::calculateRoute(const vector<vector<int>>& distanceMatrix, 
    const vector<vector<double>>& slopeMatrix, 
    const MST& spanning_tree,
    const vector<int>& readyTimes, 
    const vector<int>& dueTimes, 
    const vector<int>& serviceTimes, 
    const int& capacity, 
    const vector<int>& demands,
    const vector<double>& temperatures,
    const vector<double>& rainfall) {
    
    // guard against r being empty - and then bail
    if (r.empty()) {
        total_distance = 0;
        distances.clear();
        slopes.clear();
        speeds.clear();
        arrival_times.clear();
        departure_times.clear();
        late_flags.clear();
        loadings.clear();
        total_late = 0;
        max_loading = 0;
        return;
    }

    // *** now for the heavy lifting! ***
    depot_departure_time = 0;
    
    // distances
    distances = std::vector<int>(r.size() + 1, 0);  // +1 for depot return
    distances[0] = distanceMatrix[0][r[0]];
    total_distance = distances[0];
    for (size_t i = 1; i < r.size(); i++) {
        distances[i] = distances[i-1] + distanceMatrix[r[i-1]][r[i]];
        total_distance += distances[i];
    }
    distances[r.size()] = distances[r.size() - 1] + distanceMatrix[r.back()][0];    // Add return to depot
    total_distance += distances[r.size()];
    
    // distances
    slopes = std::vector<double>(r.size() + 1, 0);  // +1 for depot return
    slopes[0] = slopeMatrix[0][r[0]];
    for (size_t i = 1; i < r.size(); i++) {
        slopes[i] = slopeMatrix[r[i-1]][r[i]];
    }
    slopes[r.size()] = slopeMatrix[r.back()][0];    // Add return to depot
    
    // speeds, arrival times & departure times
    speeds = std::vector<int>(r.size() + 1, 0);
    arrival_times = std::vector<int>(r.size() + 1, 0);
    departure_times = std::vector<int>(r.size() + 1, 0);

    // First leg: depot to first node
    int leg_distance = distances[0];
    auto [speed, speed_factor] = speedAndFactor(leg_distance);
    speeds[0] = speed;
    int first_leg_time = std::ceil(static_cast<double>(leg_distance) / speeds[0]);
    
    
    arrival_times[0] = depot_departure_time + first_leg_time;
    departure_times[0] = std::max(
        (arrival_times[0] + serviceTimes[r[0]]),
        (readyTimes[r[0]] + serviceTimes[r[0]])
    );

    // Intermediate legs
    for (size_t i = 1; i < r.size(); i++) {
        int leg_distance = distanceMatrix[r[i - 1]][r[i]];
        auto [speed, speed_factor] = speedAndFactor(leg_distance);
        speeds[i] = speed;
        int journey_time = std::ceil(static_cast<double>(leg_distance) / speeds[i]);

        arrival_times[i] = departure_times[i-1] + journey_time;
        
        departure_times[i] = std::max(
            (arrival_times[i] + serviceTimes[r[i]]),
            (readyTimes[r[i]] + serviceTimes[r[i]])
        );
    }

    // Return to depot
    int leg_distance_back = distanceMatrix[r.back()][0];
    auto [speed_home, speed_home_factor] = speedAndFactor(leg_distance);
    speeds[r.size()] = speed_home;
    int back_time = std::ceil(static_cast<double>(leg_distance_back) / speeds[r.size()]);
    arrival_times[r.size()] = departure_times[r.size() - 1] + back_time;

    // check the arrival times
    late_flags = std::vector<int>(r.size()+1, 0);
    lateness_per_stop = std::vector<int>(r.size()+1, 0);
    total_late = 0;
    is_feasible = 1;
    for (size_t i = 0; i < r.size(); i++) {         // all legs on the route
        if (departure_times[i] > dueTimes[r[i]]) {
            late_flags[i] = 1;
            lateness_per_stop[i] = departure_times[i] - dueTimes[r[i]];
            total_late += 1;
            is_feasible = 0;
        }
    }
    if (departure_times[r.size()] > dueTimes[0]) {    // and the depot
        late_flags[r.size()] = 1;
        lateness_per_stop[r.size()] = departure_times[r.size()] - dueTimes[0];
        total_late += 1;
        is_feasible = 0;
    }
    
    // calculate the loads - load back to depot is immaterial - it should be zero
    loadings = std::vector<int>(r.size(), 0);
    max_loading = 0;
    for (size_t i = 0; i < r.size(); i++) {
        if (i == 0) {
            loadings[i] = demands[r[i]];
        } else {
            loadings[i] = loadings[i-1] + demands[r[i]];
        }
        if (loadings[i] > max_loading) {
            max_loading = loadings[i];
        }
    }
    std::reverse(loadings.begin(), loadings.end()); // reverse the loadings vector - we actually drop off so this needs to reduce
    loadings.push_back(0);
    if (max_loading > capacity) {
        is_feasible = 0;
    }
    
    
    // AND NOW for the SoC calculations
    soc_detail.clear();
    soc_summary = determineSoC_2(distanceMatrix, 
        slopeMatrix, 
        spanning_tree,
        readyTimes, 
        dueTimes, 
        serviceTimes,
        temperatures,
        rainfall);
    
}
*/

vector<SoCEntry> Route::getSoCDetail() const {
    return soc_detail;
}


void Route::logDetailedRoute(const string& filename, 
    const vector<vector<int>>& distanceMatrix, 
    const vector<vector<double>>& slopeMatrix, 
    const MST& spanning_tree,
    const vector<int>& readyTimes, 
    const vector<int>& dueTimes, 
    const vector<int>& serviceTimes,
    const vector<int>& demands,
    const vector<double>& temperatures,
    const vector<double>& rainfall) {
    
//    soc_summary = determineSoC_2(distanceMatrix, 
//        slopeMatrix, 
//        readyTimes, 
//        dueTimes, 
//        serviceTimes,
//        temperatures,
//        rainfall);

    soc_detail.clear();
    simulateRouteTimelineMST(distanceMatrix, 
            slopeMatrix, 
            spanning_tree,
            readyTimes, 
            dueTimes, 
            serviceTimes, 
            CAPACITY, 
            demands,
            temperatures,
            rainfall);
        
    // Before opening in append mode, check if file exists
    std::fstream checkFile(filename, std::ios::in);
    bool writeHeader = !checkFile.good();
    checkFile.close();

    std::fstream file(filename, std::ios::app);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open log file.");
    }
    if (writeHeader) {
        file << "route_number,minute,soc_used,cumulative_soc,activity,timeliness,speed,dist_per_minute,step_distance,slope,temperature,precipitation,loading,leg_location_id,step_location_id\n";
        //file << "route_number,minute,soc_used,cumulative_soc,activity,timeliness,speed,dist_per_minute,slope,temperature,rainfall,location_id\n";
    }

    file << std::fixed << std::setprecision(6);  // optional, but improves float readability

    for (const auto& entry : soc_detail) {
        file << route_number << "," 
             << entry.minute << ","
             << entry.soc_used << "," 
             << entry.cumulative_soc << ","
             << entry.activity << "," 
             << entry.timeliness << "," 
             << entry.speed << ","
             << entry.dist_per_minute << "," 
             << entry.step_distance << ","
             << entry.slope << ","
             << entry.temperature << "," 
             << entry.precipitation << "," 
             << entry.loading << ","
             << entry.leg_location_id << ","
             << entry.step_location_id << endl;
    }
    file.close();
}

int Route::getRouteFeasibility() const {
    return is_feasible;
}

tuple<int, int> Route::getSimulationTimeHoursMins(int t) {
    
    // get hours and minutes -> will go wrong if over 1440!
    int h = t / 60;
    int m = t % 60;
    return make_tuple(h, m);
     
    //return make_tuple(23, 59); //  dummy value for test routes
}

double Route::speedFactor(double speed) {
    
    if (speed == 0.0) {
        return 0.0;
    }
    if (speed <= 25.0) {  // < 2 km - urban only - 20–30 km/h
        return 0.9;
    }
    if (speed < 35.0) {  // < 5 km - mixed low-speed - 30–40 km/h
        return 1.0;
    }
    return 1.1;  // else - includes high-speed leg - 40–60+ km/h
}

double Route::hvacDrawKW(double temperature) {
    if (temperature < 5.0) return 1.2;   // heavy heating needed
    if (temperature < 10.0) return 0.8;  // moderate heating
    if (temperature > 30.0) return 1.0;  // heavy cooling
    if (temperature > 25.0) return 0.6;  // light cooling
    return 0.2; // mild or no HVAC needed
}

double Route::speedModifier(double slope, double rain_intensity, int load_kg) {
    
    double mod = 1.0;

    // Slope impact
    if (slope > 0.05)       mod *= 0.9;   // steep uphill
    else if (slope < -0.05) mod *= 1.05;  // steep downhill
    else if (slope > 0.02)  mod *= 0.95;
    else if (slope < -0.02) mod *= 1.025;

    // Rainfall impact (assuming rain_intensity in mm/hour)
    if (rain_intensity > 8)        mod *= 0.7;   // heavy rain
    else if (rain_intensity > 2)   mod *= 0.85;  // moderate rain
    else if (rain_intensity > 0)   mod *= 0.95;  // light rain

    // Load impact (linear-ish)
    mod *= 1.0 - std::min(0.2, (load_kg * LOAD_KG_CONV) / 2000.0);  // max 20% speed penalty for full load

    return std::max(mod, 0.3);  // prevent absurdly slow speeds
}

int Route::getEndTime() const {
    if (arrival_times.size() > 0) {
        return arrival_times[arrival_times.size()-1];
    }
    return 0;
}

double Route::computeAccelerationImpulse(double load, double target_speed_kmh) {
    const double base_mass_kg = 2500.0;   // e.g. eSprinter empty
    double total_mass_kg = base_mass_kg + (load * LOAD_KG_CONV);

    double v = target_speed_kmh / 3.6;  // convert km/h to m/s
    double energy_joules = 0.5 * total_mass_kg * v * v;

    // Convert to kWh
    double energy_kwh = energy_joules / 3.6e6;

    // Convert to SoC units (relative to full battery)
    return energy_kwh / FULL_BATTERY_KWH;
}

int Route::adjustedServiceTime(int base_minutes, double rainfall_mm_per_hr) {
    if (rainfall_mm_per_hr > 8.0) return base_minutes + 10;
    if (rainfall_mm_per_hr > 2.0) return base_minutes + 5;
    return base_minutes;
}

int Route::getTotalLate() {
    int total_late = 0;
    for (size_t i = 0; i < late_flags.size(); i++) {
        total_late += late_flags[i];
    }
    return total_late;
}

int Route::getTotalLoad() {
    if (loadings.size() > 0) {
        return std::max(0, loadings[0]);
    }
    return 0;
}

int Route::getTotalLoad() const {
    if (loadings.size() > 0) {
        return std::max(0, loadings[0]);
    }
    return 0;
}

int Route::getTotalDistance() const {
    if (distances.size() > 0) {
        return distances.back();
    }
    return 0;
}

double Route::computeRegen(double slope, double speed, double mass) {
    return 0.0;
}



double Route::getLatenessPenaltyMultiplier() {
    return lateness_penalty_multiplier;
}

void Route::setLatenessPenaltyMultiplier(double lpm) {
    lateness_penalty_multiplier = lpm;
}

double Route::getCapacityPenaltyMultiplier() {
    return capacity_penalty_multiplier;
}

void Route::setCapacityPenaltyMultiplier(double cpm) {
    capacity_penalty_multiplier = cpm;
}


double Route::costRoute(const vector<vector<int>>& distanceMatrix, 
    const vector<vector<double>>& slopeMatrix, 
    const MST& spanning_tree,
    const vector<int>& readyTimes, 
    const vector<int>& dueTimes, 
    const vector<int>& serviceTimes, 
    const int& capacity, 
    const vector<int>& demands,
    const vector<double>& temperatures,
    const vector<double>& rainfall) {
    
    simulateRouteTimelineMST(distanceMatrix, slopeMatrix, spanning_tree, readyTimes, dueTimes, serviceTimes, capacity, demands, temperatures, rainfall);
    
    int late_penalty = total_late * lateness_penalty_multiplier;
    int capacity_penalty = std::max(0, max_loading - CAPACITY) * capacity_penalty_multiplier;

    return total_distance + late_penalty + capacity_penalty;
}
    
/*
double Route::costRouteAdaptive(
    const vector<vector<int>>& distanceMatrix,
    const vector<vector<double>>& slopeMatrix,
    const MST& spanning_tree,
    const vector<int>& readyTimes,
    const vector<int>& dueTimes,
    const vector<int>& serviceTimes,
    const int& capacity,
    const vector<int>& demands,
    const vector<double>& temperatures,
    const vector<double>& rainfall,
    double w_distance,
    double w_lateness,
    double w_capacity) {
        
    calculateRoute(distanceMatrix, slopeMatrix, spanning_tree, readyTimes, dueTimes, serviceTimes, capacity, demands, temperatures, rainfall);


    double overload_cost = std::max(0, max_loading - capacity);

    return w_distance * total_distance +
           w_lateness * pow(total_late, 1.2) +
           w_capacity * overload_cost;
}
*/

int Route::getCapacity() {
    return CAPACITY;
}

/*
double Route::hillclimbRoute(const vector<vector<int>>& distanceMatrix, 
    const vector<vector<double>>& slopeMatrix, 
    const MST& spanning_tree,
    const vector<int>& readyTimes, 
    const vector<int>& dueTimes, 
    const vector<int>& serviceTimes, 
    const int& capacity, 
    const vector<int>& demands,
    const vector<double>& temperatures,
    const vector<double>& rainfall,
    double w_distance, 
    double w_lateness, 
    double w_capacity) {
    
    double bsf = costRouteAdaptive(distanceMatrix, slopeMatrix, spanning_tree, readyTimes, dueTimes, serviceTimes, capacity, demands, temperatures, rainfall, w_distance, w_lateness, w_capacity);
    
    int change = 1;
    while (change == 1) {
        change = 0;
        
        for (size_t i = 0; i < r.size(); i++) {
            for (size_t j = 0; j < r.size(); j++) {
                if (i != j) {
                    swap(r[i], r[j]);
                    double ns = costRouteAdaptive(distanceMatrix, slopeMatrix, spanning_tree, readyTimes, dueTimes, serviceTimes, capacity, demands, temperatures, rainfall, w_distance, w_lateness, w_capacity);
                    
                    if (ns < bsf) {
                        change = 1;
                        bsf = ns;
                    } else {
                        swap(r[i], r[j]);
                        calculateRoute(distanceMatrix, slopeMatrix, spanning_tree, readyTimes, dueTimes, serviceTimes, capacity, demands, temperatures, rainfall);
                    }
                }
            }
        }
    }
    
    calculateRoute(distanceMatrix, slopeMatrix, spanning_tree, readyTimes, dueTimes, serviceTimes, capacity, demands, temperatures, rainfall);  // ensure route is correct
    return bsf;
}
*/


int Route::sum(const vector<int>& vec) {
    return std::accumulate(vec.begin(), vec.end(), 0);
}

double Route::sum(const vector<double>& vec) {
    return std::accumulate(vec.begin(), vec.end(), 0);
}

void Route::twoOptSwap(size_t i, size_t k) {
    std::reverse(r.begin() + i, r.begin() + k + 1);
}

int Route::size() {
    return r.size();
}

double Route::twoOptSearch(const vector<vector<int>>& distanceMatrix,
    const vector<vector<double>>& slopeMatrix,
    const MST& spanning_tree,
    const vector<int>& readyTimes,
    const vector<int>& dueTimes,
    const vector<int>& serviceTimes,
    const int& capacity,
    const vector<int>& demands,
    const vector<double>& temperatures,
    const vector<double>& rainfall,
    bool shuffle_order,
    int max_no_improvement) {
    
    double bsf = costRoute(distanceMatrix, slopeMatrix, spanning_tree, readyTimes, dueTimes, serviceTimes, capacity, demands, temperatures, rainfall);
    int no_improvement_counter = 0;
    int max_segment_length = std::max(2, static_cast<int>(r.size() / 2));

    std::vector<size_t> indices(r.size());
    std::iota(indices.begin(), indices.end(), 0);

    std::random_device rd;
    std::mt19937 g(rd()); // Mersenne Twister RNG

    while (no_improvement_counter < max_no_improvement) {
        bool improved = false;

        if (shuffle_order)
            std::shuffle(indices.begin(), indices.end(), g);

        for (size_t i = 0; i < indices.size() - 1; ++i) {
            for (size_t k = i + 1; k < std::min(indices.size(), i + max_segment_length); ++k) {
                twoOptSwap(indices[i], indices[k]);

                double new_cost = costRoute(distanceMatrix, slopeMatrix, spanning_tree, readyTimes, dueTimes, serviceTimes, capacity, demands, temperatures, rainfall);
                if (new_cost < bsf) {
                    bsf = new_cost;
                    improved = true;
                    no_improvement_counter = 0;  // reset counter
                } else {
                    twoOptSwap(indices[i], indices[k]); // Undo
                    simulateRouteTimelineMST(distanceMatrix, slopeMatrix, spanning_tree, readyTimes, dueTimes, serviceTimes, capacity, demands, temperatures, rainfall);
                }
            }
        }

        if (!improved) {
            ++no_improvement_counter;
        }
    }

    return bsf;
}


// for debugging - print a vector to the console
void Route::print_vector(const vector<int>& vec) {
    for (const auto& item : vec) {
        cout << item << " ";
    }
    cout << "\n";
}

int Route::addStore(int store) { // add a store to the Route

    // going defensive!
    if (std::find(r.begin(), r.end(), store) != r.end()) {
        cerr << "trying to add store " << store << " to route [";
        for (size_t x = 0; x < r.size(); x++) {
            cerr << r[x] << " ";
        }
        cerr << "] BUT already there!" << endl;
        return -1; // Duplicate store
    } 
    r.push_back(store);
    return 0;
};

int Route::addStore(int store, int position) { // add a store to the Route

    // going defensive!
    if (std::find(r.begin(), r.end(), store) != r.end()) {
        cerr << "trying to add store " << store << " to route [";
        for (size_t x = 0; x < r.size(); x++) {
            cerr << r[x] << " ";
        }
        cerr << "] BUT already there!" << endl;
        return -1; // Duplicate store
    } 
    if (position < 0 || position > (int)r.size()) {
        std::cerr << "Invalid position " << position << " for route size " << r.size() << std::endl;
        return -2;  // Invalid position
    }

    r.insert(r.begin() + position, store);
    return 0;  // Success
};

int Route::removeStore(int store) { // remove a store from the Route

    // must not leave a route empty
    if (r.size() == 1) {
        cerr << "trying to remove store " << store << " from route [";
        for (size_t x = 0; x < r.size(); x++) {
            cerr << r[x] << " ";
        }
        cerr << "]" << endl;
        return -1;
    }
    
    for(size_t x = 0; x < r.size(); x++) {
        if(r[(int)x] == store) {
            r.erase(r.begin() + (int)x);
            return 0;
        }
    }
    cerr << "trying to remove store " << store << " from route [";
    for (size_t x = 0; x < r.size(); x++) {
        cerr << r[x] << " ";
    }
    cerr << "]" << endl;
    
    return -1; // store not found
};

string Route::getStr() {
    string r_str = std::to_string(route_number) + " : (" + std::to_string(total_distance) + ") : ";
    r_str += "route ";
    for (size_t i = 0; i < r.size(); i++) {
        r_str += std::to_string(r[i]) + " ";
    }
    return r_str;
};

void Route::display() {
    cout << route_number << " : (" << total_distance << ") : ";
    cout << "route ";
    for (size_t i = 0; i < r.size(); i++) {
        cout << r[i] << " ";
    }
    cout << " : ";
    cout << "distances ";
    for (size_t i = 0; i < distances.size(); i++) {
        cout << distances[i] << " ";
    }
    //cout << " : ";
    //cout << "slopes ";
    //for (size_t i = 0; i < slopes.size(); i++) {
    //    cout << slopes[i] << " ";
    //}
    
    cout << " : ";
    cout << "arrival times ";
    for (size_t i = 0; i < arrival_times.size(); i++) {
        cout << arrival_times[i] << " ";
    }
    
    cout << " : ";
    cout << "departure times ";
    for (size_t i = 0; i < departure_times.size(); i++) {
        cout << departure_times[i] << " ";
    }
    
    cout << " : ";
    cout << "late flags ";
    for (size_t i = 0; i < late_flags.size(); i++) {
        cout << late_flags[i] << " ";
    }
    cout << " : ";
    cout << "loadings ";
    for (size_t i = 0; i < loadings.size(); i++) {
        cout << loadings[i] << " ";
    }
    cout << " : " << max_loading ;
    
    cout << " : ";
    cout << "SoC levels ";
    for (size_t i = 0; i < soc_summary.size(); i++) {
        cout << soc_summary[i] << " ";
    }
    //cout << " : " << soc_levels[soc_levels.size()-1] ;
    
    cout << endl;
};

void Route::display() const {
    cout << route_number << " : (" << total_distance << ") : ";
    cout << "route ";
    for (size_t i = 0; i < r.size(); i++) {
        cout << r[i] << " ";
    }
    cout << " : ";
    cout << "distances ";
    for (size_t i = 0; i < distances.size(); i++) {
        cout << distances[i] << " ";
    }
    //cout << " : ";
    //cout << "slopes ";
    //for (size_t i = 0; i < slopes.size(); i++) {
    //    cout << slopes[i] << " ";
    //}
    
    cout << " : ";
    cout << "arrival times ";
    for (size_t i = 0; i < arrival_times.size(); i++) {
        cout << arrival_times[i] << " ";
    }
    
    cout << " : ";
    cout << "departure times ";
    for (size_t i = 0; i < departure_times.size(); i++) {
        cout << departure_times[i] << " ";
    }
    
    cout << " : ";
    cout << "late flags ";
    for (size_t i = 0; i < late_flags.size(); i++) {
        cout << late_flags[i] << " ";
    }
    cout << " : ";
    cout << "loadings ";
    for (size_t i = 0; i < loadings.size(); i++) {
        cout << loadings[i] << " ";
    }
    cout << " : " << max_loading ;
    
    cout << " : ";
    cout << "SoC levels ";
    for (size_t i = 0; i < soc_summary.size(); i++) {
        cout << soc_summary[i] << " ";
    }
    //cout << " : " << soc_levels[soc_levels.size()-1] ;
    
    cout << endl;
};


double Route::getFinalSoC() {
    return soc_summary[soc_summary.size()-1] ;
}

