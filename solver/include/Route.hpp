#pragma once

#include <vector>
#include <tuple>
#include <iostream>
#include <cstddef>
#include <bits/stdc++.h>

#include <random>

#include "MST.hpp"
#include "StepResult.hpp"
#include "LegResult.hpp"
#include "SoCEntry.hpp"

using namespace std;

class Route {

    private:
        int route_number;
        vector<int> r;
        
        int is_feasible;
        vector<int> distances;
        vector<double> slopes;
        vector<int> speeds;
        
        int total_distance;
        vector<int> arrival_times;
        int depot_departure_time;
        vector<int> departure_times;
        vector<int> late_flags;
        vector<int> lateness_per_stop;
        vector<int> earliness_per_stop;
        int total_late;
        vector<int> loadings;
        int max_loading;
        
        
    
        vector<double> soc_summary;
        std::vector<SoCEntry> soc_detail;
        
        double hvacDrawKW(double temp);
        
        
        double lateness_penalty_multiplier = 1000.0;  // penalty for being late
        double lateness_util = 0.0; // or "feature utility" if you generalize
        
        
        
        int CAPACITY = 200; // capacity of the truck
        double capacity_penalty_multiplier = 1000.0;  // penalty for exceeding capacity
        double capacity_util = 0.0; // or "feature utility" if you generalize
        
        const double LOAD_KG_CONV = 5.0;
        const double FULL_BATTERY_KWH = 40.0;
        
        const bool USE_ACCELERATION_IMPULSE = true;
        const bool OPTIMISE_DEPOT_DEPARTURE = true;
        
        // e-bike settings
        //const double FULL_BATTERY_KWH = 0.5;
        //const double ENERGY_PER_KM_KWH = 0.1;
        const double ENERGY_PER_KM_KWH = 0.4;
        const double BASE_SOC = ENERGY_PER_KM_KWH / 1000.0 / FULL_BATTERY_KWH;
        const bool REGEN_BRAKING = true;
        const double BASE_REGEN = 0.0001;

        int sum(const vector<int>& vec) ;
        double sum(const vector<double>& vec) ;
        void print_vector(const vector<int>& vec);
        void twoOptSwap(size_t i, size_t k);

        tuple<int, int> getSimulationTimeHoursMins(int t);
        
        double getHourlyTemperature(int timestamp, vector<double> temperatures);
        double getHourlyRainfall(int timestamp, vector<double> rainfall);
        double slopeFactor(double slope);
        double temperatureFactor(double temp);
        double loadFactor(int load);
        std::pair<double, double> speedAndFactor(double distance);
        double speedFactor(double speed);
        double speedModifier(double slope, double rain_intensity, int load_kg);
        double computeAccelerationImpulse(double load_kg, double target_speed_kmh);
        double computeRegen(double slope, double speed, double mass);
        double regenBrakingGain(double slope, double speed, double load_factor, double rainfall);
        int adjustedServiceTime(int base_minutes, double rainfall_mm_per_hr);
        
        
        int depart_depot_time = 0;
        int return_to_depot_time;
        
    public:
        Route(); // default constructor - empty route
        ~Route();
        Route(vector<int> r); // constructor to initialize the route with a vector of integers
        Route(int tp, int lp, int dp); // constructor to initialize the route
        Route(const Route& other) ; // copy constructor
            
        int size();
        
        void setRouteNumber(int nr);
        int getRouteNumber(){return route_number; }
        
        
        void calculateRoute(
            const vector<vector<int>>& distanceMatrix, 
            const vector<vector<double>>& slopeMatrix, 
            const MST& spanning_tree,
            const vector<int>& readyTimes, 
            const vector<int>& dueTimes, 
            const vector<int>& serviceTimes, 
            const int& capacity, 
            const vector<int>& demands,
            const vector<double>& temperatures,
            const vector<double>& rainfall);
        
        
        
        double costRoute(
            const vector<vector<int>>& distanceMatrix, 
            const vector<vector<double>>& slopeMatrix, 
            const MST& spanning_tree,
            const vector<int>& readyTimes, 
            const vector<int>& dueTimes, 
            const vector<int>& serviceTimes, 
            const int& capacity, 
            const vector<int>& demands,
            const vector<double>& temperatures,
            const vector<double>& rainfall);
        
        /*
        void simulateRouteTimeline(const vector<vector<int>>& distanceMatrix, 
            const vector<vector<double>>& slopeMatrix, 
            const mst& spanning_tree,
            const vector<int>& readyTimes, 
            const vector<int>& dueTimes, 
            const vector<int>& serviceTimes, 
            const int& capacity, 
            const vector<int>& demands,
            const vector<double>& temperatures,
            const vector<double>& rainfall);
        */
        
        void simulateRouteTimelineMST(const vector<vector<int>>& distanceMatrix, 
            const vector<vector<double>>& slopeMatrix, 
            const MST& spanning_tree,
            const vector<int>& readyTimes, 
            const vector<int>& dueTimes, 
            const vector<int>& serviceTimes, 
            const int& capacity, 
            const vector<int>& demands,
            const vector<double>& temperatures,
            const vector<double>& rainfall);
            
        /*
        double costRoute(
            const vector<vector<int>>& distanceMatrix,
            const vector<vector<double>>& slopeMatrix,
            const mst& spanning_tree,
            const vector<int>& ready_times,
            const vector<int>& due_times,
            const vector<int>& service_times,
            int vehicle_capacity,
            const vector<int>& demands,
            const vector<double>& temperatures,
            const vector<double>& rainfall,
            const vector<int>& proposedRoute  // <- new param
        ) const;
        */
        
        /*
        double costRouteAdaptive(
            const vector<vector<int>>& distanceMatrix,
            const vector<vector<double>>& slopeMatrix,
            const mst& spanning_tree,
            const vector<int>& readyTimes,
            const vector<int>& dueTimes,
            const vector<int>& serviceTimes,
            const int& capacity,
            const vector<int>& demands,
            const vector<double>& temperatures,
            const vector<double>& rainfall,
            double w_distance,
            double w_lateness,
            double w_capacity);
        

    */
    
        vector<SoCEntry> getSoCDetail() const;
        
        int getCapacity();
        int getRouteFeasibility() const;
        
        Route& operator=(const Route& other) ; // assignment operator
        void setRoute(const vector<int>& new_route) ; // set the route
        vector<int> getRoute() const;
        vector<int>& getRoute();
        
        int getRoute(int s) const;
        int& getRoute(int s);
                
        int getTotalLoad() const;
        int getTotalDistance() const;
        int getEndTime() const;
        
        double hillclimbRoute(const vector<vector<int>>& distanceMatrix, 
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
            double w_capacity);
        
        double twoOptSearch(const vector<vector<int>>& distanceMatrix,
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
            int max_no_improvement);
                        
        void display();
        void display() const;
        string getStr();
        int addStore(int store) ;
        int addStore(int store, int position) ;
        int removeStore(int store) ;
            
        double getDistance();
        double getFinalSoC();
        void logDetailedRoute(const string& filename,
            const vector<vector<int>>& distanceMatrix, 
            const vector<vector<double>>& slopeMatrix, 
            const MST& spanning_tree,
            const vector<int>& readyTimes, 
            const vector<int>& dueTimes, 
            const vector<int>& serviceTimes,
            const vector<int>& demands,
            const vector<double>& temperatures,
            const vector<double>& rainfall);
            
        int getTotalLoad();
        int getTotalLate();
        const std::vector<int>& getLateFlags() const { return late_flags; }
        const std::vector<int>& getLatenessPerStop() const { return lateness_per_stop; }
        const std::vector<int>& getEarlinessPerStop() const { return earliness_per_stop; }
        
        void setLatenessPenaltyMultiplier(double lpm);
        double getLatenessPenaltyMultiplier();
        
        void setCapacityPenaltyMultiplier(double cpm);
        double getCapacityPenaltyMultiplier();
};

