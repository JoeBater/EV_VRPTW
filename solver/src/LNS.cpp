#include "LNS.hpp"

#include <filesystem>

LNS::LNS(Instance* instance,
         string plFile,
         string vdDir)
{
    inst = instance;
    stagnationCounter = 0;
    
    VERBOSE = inst->getVerbosity();
    DEBUG = inst->getDebug();
    
    if (VERBOSE) {
        cout << "declared LNS instance with " << inst->getInstanceDataSize() << " nodes, " << inst->getRoutes().size() << " routes " 
             << VERBOSE << " VERBOSITY, " << DEBUG << " DEBUG..." << "\n";
    }
    
    progressLogFile = plFile;
    videoDir = vdDir;
    VIDEO = !videoDir.empty();
}


LNS::~LNS()
{
}

SolutionScore LNS::runLargeNeighbourSearch(int iterations) {
    
    std::ofstream file(progressLogFile, std::ios::out | std::ios::trunc);
    file.close();  // Just creates an empty file, clearing any previous content

    std::ofstream logStream(progressLogFile, std::ios::app);  // open once at the start

    inst->calculateRoutes();
    SolutionScore bsf = evaluateSolution();
    
    string best_dir = videoDir + "/best/";
    string new_dir = videoDir + "/new/";
    if (VIDEO) {
        std::filesystem::create_directories(best_dir);
        std::filesystem::create_directories(new_dir);
    }
    
    
    for (int it = 0; it < iterations; it++) {
        
        // profiling start timer
//        auto start = std::chrono::steady_clock::now();
        
        vector<Route> backup = inst->getRoutes();  // Deep clone includes soc_detail, etc.
        SolutionScore backupScore = bsf;           // Save current score
        
        // dynamically alter destroy strength
        double pct = static_cast<double>(it) / iterations;
        if (pct < 0.01) {
            // 🧨 Rapid Descent
            destroyStrength = 0.3;
        } else if (pct < 0.10) {
            // 🎯 Constraint Play
            destroyStrength = 0.2;
        } else if (pct < 0.50) {
            // 🌫️ Plateau Drift
            destroyStrength = 0.1;
        } else {
            // 🪶 Tail Refinement
            destroyStrength = 0.05; // conservative with slight variation
        }
        
        string destroy_method = destroy();
        string repair_method = repair();
        
        //bool chk = inst->checkAllStoresVisitedOnce();
        //cerr << chk << " all stores visited" << endl;
        
        SolutionScore newCost = evaluateSolution();
        
        if (VERBOSE) {
            cout << "iteration " << it << "\t" << "bsf = " << bsf.total_cost << " (" << bsf.total_distance << " " << bsf.total_late << " " << bsf.total_overload << ")\tns = " << newCost.total_cost << "\n";
        }
        
        // Monitoring
        if (MONITOR) {
            logStream << it << "," 
                      << bsf.total_cost << "," 
                      << newCost.total_cost << "," 
                      << (newCost.total_cost < bsf.total_cost) << "," 
                      << destroy_method << "," 
                      << repair_method << "\n";
        }
    
        if (VIDEO) {
            std::string newfilename = new_dir + std::to_string(it) + ".txt";
            inst->writeRoutesToFile(newfilename);
        }
    
    
        if (newCost.total_cost < bsf.total_cost) {
            bsf = newCost;
        } else {
            inst->setRoutes(backup);           // Already computed routes
            bsf = backupScore;                 // Restore score without re-evaluating
        }
        
        if (VIDEO) {
            std::string bestfilename = best_dir + std::to_string(it) + ".txt";
            inst->writeRoutesToFile(bestfilename);
        }
        
        // profiling
//        auto end = std::chrono::steady_clock::now();
//        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
//        std::cout << "[PROFILE] Iteration took " << duration << " ms\n";
    }
    
    twoOptAllRoutes();
    SolutionScore endCost = evaluateSolution();
    cerr << bsf.total_cost << " " << endCost.total_cost << endl;
    
    if (VIDEO) {
        std::string bestfilename = best_dir + std::to_string(iterations) + ".txt";
        inst->writeRoutesToFile(bestfilename);
    }
        
    logStream.close();
    cout << endl;    

    //string detailedRouteLogFile = "C:/Users/user/ML/Optimisation/VRPTW/detailed_route_log_1_1.csv";
    //inst->logDetailedRoutes(detailedRouteLogFile);
        
    return bsf;
}

SolutionScore LNS::evaluateSolution() {
    
    inst->calculateRoutes();
    
    int dist = inst->getTotalDistance();
    int late = inst->getTotalLate();  // number of sites late
    int tardiness = inst->getTotalTardiness();
    int overload = inst->getTotalOverload();
    int late_penalty = (int)(w_lateness * late + w_tardiness * tardiness);

    return {
        (w_distance * dist),
        late_penalty,
        (int)(w_capacity * overload),
        (w_distance * dist) + late_penalty + (int)(w_capacity * overload)
    };
}

Quadrant LNS::getQuadrant(int x, int y, int depotX, int depotY) {
    if (x >= depotX && y >= depotY) return NE;
    if (x <  depotX && y >= depotY) return NW;
    if (x <  depotX && y <  depotY) return SW;
    return SE;
}


void LNS::twoOptAllRoutes() {
    
    for (auto& routeObj : inst->getRoutes()) {
        auto& route = routeObj.getRoute();
        bool improved = true;

        while (improved) {
            improved = false;

            for (size_t i = 1; i < route.size() - 1; ++i) {
                for (size_t j = i + 1; j < route.size(); ++j) {
                    std::vector<int> candidate = route;
                    std::reverse(candidate.begin() + i, candidate.begin() + j + 1);

                    if (inst->isTimeWindowFeasible(candidate)) {
                        int before = inst->getDistance(route[i - 1], route[i]) +
                                     inst->getDistance(route[j], route[j + 1]);

                        int after = inst->getDistance(candidate[i - 1], candidate[i]) +
                                    inst->getDistance(candidate[j], candidate[j + 1]);

                        if (after < before) {
                            route = candidate;
                            improved = true;
                        }
                    }
                }
            }
        }
    }
}

        
string LNS::destroy() {
    
    string destroy_method = "";
    bool useRandomDestroy = false;
    bool useLateDestroy = false;
    bool useAnchorLateDestroy = false;
    bool useZigZagDestroy = false;
    bool useShawDestroy = false;
    bool useRouteLengthDestroy = false;
    bool useExcessiveEarlyArrivalDestroy = false;
    
    double dice = uniformDist(rng);
    if (dice < 0.1428) {
        useRandomDestroy = true; 
        destroy_method = "RandomDestroy";
    } else if (dice < 0.2857) {
        useLateDestroy = true;
        destroy_method = "LateDestroy";
    } else if (dice < 0.4285) {
        useAnchorLateDestroy = true;
        destroy_method = "AnchorLateDestroy";
    } else if (dice < 0.5714) {
        useZigZagDestroy = true;
        destroy_method = "ZigZagDestroy";
    } else if (dice < 0.7142) {
        useShawDestroy = true;
        destroy_method = "ShawDestroy";
    } else if (dice < 0.8571) {
        useRouteLengthDestroy = true;
        destroy_method = "RouteLengthDestroy";
    }
    else {
        useExcessiveEarlyArrivalDestroy = true;
        destroy_method = "ExcessiveEarlyArrivalDestroy";
    }

//    if (VERBOSE) {
//        std::cout << "[ALNS] Destroy phase: " << (useRandomDestroy ? "Random" : "Late") << " with strength " << (destroyStrength * 100.0) << "%\n";
//    }

    inst->calculateRoutes();  // Make sure everything about a route e.g. lateness flags/vectors are updated!
    int num_routes = inst->getRoutes().size();
    if (num_routes < 2) {
        std::cerr << "[ALNS] Not enough routes to perform destroy phase\n";
        exit(-1);
    }

    removed_stores.clear();

    if (useRandomDestroy) {
        // ---- RANDOM DESTROY ----
        for (size_t t = 0; t < inst->getRoutes().size(); t++) {
            auto& r = inst->getRoutes()[t];

            for (int s = r.size() - 1; s >= 0; s--) {
                double p = uniformDist(rng);
                if (p < destroyStrength && r.size() > 1) {
                    int store = r.getRoute()[s];
                    removed_stores.push_back(store);

                    if (r.removeStore(store) != 0) {
                        std::cerr << "[ALNS] Failed to remove store " << store 
                                  << " from route " << t << "\n";
                        exit(-1);
                    }
                }
            }
        }
    } 
    if (useLateDestroy) {
        // ---- LATE DESTROY ----

        struct LateStop {
            int routeIdx;
            int stopIdx;
            int lateness;  // Number of minutes late
            int storeID;   // Actual store number (global ID)
        };

        std::vector<LateStop> latenessList;

        for (size_t t = 0; t < inst->getRoutes().size(); t++) {
            auto& r = inst->getRoutes()[t];
            const auto& latenessVec = inst->getLatenessPerStop(t);

            for (int s = 0; s < r.size(); s++) {
                if (latenessVec[s] > 0) {
                    latenessList.push_back({
                        static_cast<int>(t),
                        s,
                        latenessVec[s],
                        r.getRoute()[s]  // Store ID
                    });
                }
            }
        }

//        if (latenessList.empty()) {
//            if (VERBOSE) std::cout << "[ALNS] No late stops found, skipping late destroy.\n";
//            return;
//        }

        // Sort by most late first
        std::sort(latenessList.begin(), latenessList.end(),
                  [](const LateStop& a, const LateStop& b) {
                      return a.lateness > b.lateness;  // Descending
                  });

        int numToRemove = std::ceil(latenessList.size() * destroyStrength);
        numToRemove = std::max(1, numToRemove);

        for (int i = 0; i < numToRemove && i < (int)latenessList.size(); i++) {
            int routeIdx = latenessList[i].routeIdx;
            int storeID  = latenessList[i].storeID;

            auto& r = inst->getRoutes()[routeIdx];

            // Defensive check: is this store still in this route?
            if (std::find(r.getRoute().begin(), r.getRoute().end(), storeID) == r.getRoute().end()) {
                if (VERBOSE) {
                    std::cerr << "[ALNS] Warning: Store " << storeID 
                              << " no longer in route " << routeIdx 
                              << ", skipping removal.\n";
                }
                continue;
            }

            removed_stores.push_back(storeID);

            if (r.removeStore(storeID) != 0) {
                std::cerr << "[ALNS] Failed to remove store " << storeID 
                          << " from route " << routeIdx << "\n";
                exit(-1);
            }
        }
    }
    
    if (useAnchorLateDestroy) {
        
        while ((int)removed_stores.size() < (int)(100.0*destroyStrength)) {
            
            // run through the routes
            int change = 0;
            for (size_t t = 0; t < inst->getRoutes().size(); t++) {
                auto& r = inst->getRoutes()[t];
                if (r.size() > 1) { // guard to ensure removal does not leave a route empty
                    const auto& latenessVec = inst->getLatenessPerStop(t);

                    for (int s = 0; s < r.size(); s++) {
                        int store = r.getRoute()[s];
                        if (latenessVec[s] > 0) {
                            removed_stores.push_back(store);
                            r.removeStore(store);
                            change = 1;
                            break;
                        }
                    }
                }
            }
            // no more late stores so escape even if we haven't hit target
            if (change == 0) {
                break;
            }
        }
    }
    
    if (useExcessiveEarlyArrivalDestroy) {
        
        int EXCESSIVE_EARLY_FACTOR = 2;
            
        // run through the routes
        //int change = 0;
        for (size_t t = 0; t < inst->getRoutes().size(); t++) {
            auto& r = inst->getRoutes()[t];
            if (r.size() > 1) { // guard to ensure removal does not leave a route empty
            
                
            
                const auto& earlinessVec = inst->getEarlinessPerStop(t);

                for (int s = 0; s < r.size(); s++) {
                    int store = r.getRoute()[s];
                    if (earlinessVec[s] > EXCESSIVE_EARLY_FACTOR * inst->getServiceTimeSite(s)) {
                        removed_stores.push_back(store);
                        r.removeStore(store);
                        //change = 1;
                        //break;
                    }
                }
            }
        }

        //cerr << "Excessive early site destroy - removed " << removed_stores.size() << " sites." << endl;
    }
    
    
    if (useZigZagDestroy) {
        
        StoreData depot = inst->getInstanceData()[0];
        
        vector<pair<int, int>> zig_zag_routes;
        int total_zig_zag_distance = 1; // ensure no divide ny zero later
        
        for (size_t t = 0; t < inst->getRoutes().size(); t++) {
            
            auto& r = inst->getRoutes()[t].getRoute();

            int score = 0;
            for (size_t s = 1; s < r.size(); s++) {
                StoreData store_q1 = inst->getInstanceData()[r[s-1]];
                StoreData store_q2 = inst->getInstanceData()[r[s]];
                Quadrant q1 = getQuadrant(store_q1.x, store_q1.y, depot.x, depot.y);
                Quadrant q2 = getQuadrant(store_q2.x, store_q2.y, depot.x, depot.y);
                
                if (q1 != q2) {
                    score += inst->getDistance(r[s - 1], r[s]);
                }
            }
            
            
            zig_zag_routes.push_back(std::make_pair(static_cast<int>(t), score));
            total_zig_zag_distance += score;
        }
        
        std::sort(zig_zag_routes.begin(), zig_zag_routes.end(),
                  [](const pair<int, int>& a, const pair<int, int>& b) {
                      return a.second > b.second;  // Descending
                  });
        
        int totalToRemove = std::ceil(destroyStrength * inst->getInstanceDataSize());
        int removed = 0;

        for (const auto& [routeIdx, score] : zig_zag_routes) {
            if (score == 0) continue;

            auto& r = inst->getRoutes()[routeIdx];
            const auto& routeVec = r.getRoute();

            int numToRemove = std::ceil(totalToRemove * (static_cast<double>(score) / total_zig_zag_distance));
            numToRemove = std::min(numToRemove, static_cast<int>(r.size()) - 1);

            int removedHere = 0;

            // Optional: remove based on quadrant transitions rather than just from head
            for (size_t i = 1; i < routeVec.size() && removedHere < numToRemove; ++i) {
                Quadrant q1 = getQuadrant(inst->getInstanceData()[routeVec[i - 1]].x,
                                          inst->getInstanceData()[routeVec[i - 1]].y, depot.x, depot.y);
                Quadrant q2 = getQuadrant(inst->getInstanceData()[routeVec[i]].x,
                                          inst->getInstanceData()[routeVec[i]].y, depot.x, depot.y);

                if (q1 != q2) {
                    removed_stores.push_back(routeVec[i]);
                    r.removeStore(routeVec[i]);
                    removedHere++;
                    removed++;
                }
            }

            if (removed >= totalToRemove) break;
        }


    }
    
    if (useShawDestroy) {
        
        // find the most late store
        struct LateStop {
            int routeIdx;
            int stopIdx;
            int lateness;  // Number of minutes late
            int storeID;   // Actual store number (global ID)
        };

        std::vector<LateStop> latenessList;

        for (size_t t = 0; t < inst->getRoutes().size(); t++) {
            auto& r = inst->getRoutes()[t];
           
            const auto& latenessVec = inst->getLatenessPerStop(t);

            for (int s = 0; s < r.size(); s++) {
                if (latenessVec[s] > 0) {
                    latenessList.push_back({
                        static_cast<int>(t),
                        s,
                        latenessVec[s],
                        r.getRoute()[s]  // Store ID
                    });
                }                
            }
        }

        int shaw_store = -1;
        if (latenessList.empty()) {
            // Fall back: select a random store from any route that isn't empty
            for (auto& r : inst->getRoutes()) {
                if (r.size() > 1) {
                    int idx = std::uniform_int_distribution<int>(0, r.size() - 1)(rng);
                    shaw_store = r.getRoute()[idx];
                    r.removeStore(shaw_store);
                    removed_stores.push_back(shaw_store);
                    break;
                }
            }
        } else {
            // Proceed with most late store as seed
            std::sort(latenessList.begin(), latenessList.end(),
                      [](const LateStop& a, const LateStop& b) {
                          return a.lateness > b.lateness;
                      });

            for (const auto& stop : latenessList) {
                auto& r = inst->getRoutes()[stop.routeIdx];
                if (r.size() > 1) {
                    shaw_store = stop.storeID;
                    r.removeStore(shaw_store);
                    removed_stores.push_back(shaw_store);
                    break;
                }
            }
        }
        
        
        int closest_store = -1;
        int closest_route = -1;
        
        while ((int)removed_stores.size() < (int)(100.0*destroyStrength)) {
            
            // run through the routes
            int closest_dist = 10e6;
            
            for (size_t t = 0; t < inst->getRoutes().size(); t++) {
                auto& r = inst->getRoutes()[t];
                if (r.size() > 1) { // no removals should result in an empty route
                    for (int s = 0; s < r.size(); s++) {
                        int store = r.getRoute()[s];
                        
                        int dist = inst->getDistance(store, removed_stores[removed_stores.size()-1]);
                        if (dist < closest_dist) {
                            closest_dist = dist;
                            closest_store = store;
                            closest_route = r.getRouteNumber();
                        }
                    }
                }
            }
        
            // and add to the removed_stores list and remove from the route
            if (closest_store > -1) {
                removed_stores.push_back(closest_store);
                auto& r = inst->getRoutes()[closest_route];
                r.removeStore(closest_store);
            } else {
                cerr << "no closest store for SHAW destroy" << endl;
            }
            
            
        }
    }
    
    if (useRouteLengthDestroy) {
        
        struct RouteLength {
            int routeIdx;
            int numStops;
        };

        std::vector<RouteLength> routeLengths;

        for (size_t t = 0; t < inst->getRoutes().size(); t++) {
            auto& r = inst->getRoutes()[t];
            routeLengths.push_back({ static_cast<int>(t), r.size() });
        }

        // Sort routes by descending length
        std::sort(routeLengths.begin(), routeLengths.end(),
                  [](const RouteLength& a, const RouteLength& b) {
                      return a.numStops > b.numStops;
                  });

        int totalToRemove = std::ceil(destroyStrength * inst->getInstanceDataSize());
        int removed = 0;

        for (size_t i = 0; i < routeLengths.size() && removed < totalToRemove; ++i) {
            int routeIdx = routeLengths[i].routeIdx;
            auto& r = inst->getRoutes()[routeIdx];
            const auto& routeVec = r.getRoute();

            // Remove up to half the stops from this route
            int numToRemove = std::min(static_cast<int>(routeVec.size()) / 2, totalToRemove - removed);
            std::vector<int> shuffledStops = routeVec;
            std::shuffle(shuffledStops.begin(), shuffledStops.end(), rng);

            for (int j = 0; j < numToRemove && removed < totalToRemove; ++j) {
                int store = shuffledStops[j];
                removed_stores.push_back(store);
                if (r.removeStore(store) != 0) {
                    std::cerr << "[ALNS] Failed to remove store " << store 
                              << " from route " << routeIdx << "\n";
                    exit(-1);
                }
                removed++;
            }
        }
    }


    
    return destroy_method;
}





string LNS::repair() {
    
    string repair_method = "";
    
    bool useRandomRepair = false;
    bool useTimeGreedyRepair = false;
    bool useShuffleRepair = false;
    bool useKSlimRegretRepair = false;
    bool useProximityReadyRepair = false;
    //bool useRandom2OptRepair = false;
    
    double dice = uniformDist(rng);
    if (dice < 0.2) {
        useRandomRepair = true; 
        repair_method = "RandomRepair";
    } else if (dice < 0.4) {
        useTimeGreedyRepair = true; 
        repair_method = "TimeGreedyRepair";
    } else if (dice < 0.6) {
        useShuffleRepair = true; 
        repair_method = "ShuffleRepair";
    } else if (dice < 0.8) {
        useKSlimRegretRepair = true; 
        repair_method = "KSlimRegretRepair";
    } else if (dice < 1.0) {
        useProximityReadyRepair = true; 
        repair_method = "ProximityReadyRepair";
    } 
    
    if (useRandomRepair) {
        auto& routes = inst->getRoutes();

        for (int store : removed_stores) {
            int route_idx = rand() % routes.size();
            auto& r = routes[route_idx];
            int pos = rand() % (r.size() + 1);

            if (r.addStore(store, pos) != 0) {
                std::cerr << "Failed to insert store " << store << " at pos " << pos << "\n";
                exit(-1);
            }
        }
    }
    
    if (useTimeGreedyRepair) {
        auto& routes = inst->getRoutes();

        for (int store : removed_stores) {
            int route_idx = rand() % routes.size();
            auto& r = routes[route_idx];
            
            int required_ready_time = inst->getReadyTimes()[store];
            
            int pos = 0;
            for (int s = 0; s < r.size(); s++) {
                int other_store = r.getRoute()[s];
                int other_ready_time = inst->getReadyTimes()[other_store];
                if (required_ready_time > other_ready_time) {
                    pos += 1;
                }
            }
            
            // no suitable position found - resort to random
            if (pos == (int)r.size()) {
                pos = rand() % (r.size() + 1);
            }

            if (r.addStore(store, pos) != 0) {
                std::cerr << "Failed to insert store " << store << " at pos " << pos << "\n";
                exit(-1);
            }
        }
    }
    
    if (useShuffleRepair) {
        auto& routes = inst->getRoutes();

        for (int store : removed_stores) {
            int required_ready_time = inst->getReadyTimes()[store];

            // Choose a random route
            int route_idx = rand() % routes.size();
            auto& routeObj = routes[route_idx];
            auto& r = routeObj.getRoute();

            // Smart pairwise swaps to improve temporal structure
            for (size_t s = 0; s < r.size() - 1; ++s) {
                int a = r[s];
                int b = r[s + 1];

                int ready_a = inst->getReadyTimes()[a];
                int ready_b = inst->getReadyTimes()[b];
                //int arrive_a = inst->getDistance(0, a);  // or use actual arrival calc if available
                int arrive_b = inst->getDistance(a, b);

                // Swap if b should logically precede a based on readiness
                if ((ready_b < ready_a) && (arrive_b <= ready_a)) {
                    std::swap(r[s], r[s + 1]);
                }
            }

            routeObj.setRoute(r);

            // Greedy insertion based on ready time alignment
            int pos = 0;
            for (size_t s = 0; s < r.size(); ++s) {
                int other_store = r[s];
                int other_ready = inst->getReadyTimes()[other_store];
                if (required_ready_time > other_ready) {
                    pos++;
                }
            }

            // Fallback if no sensible position was found
            if (pos >= (int)r.size()) {
                pos = rand() % (r.size() + 1);
            }

            if (routeObj.addStore(store, pos) != 0) {
                std::cerr << "[ShuffleRepair] Failed to insert store " << store << " at pos " << pos << "\n";
                exit(-1);
            }
        }
    }

    if (useKSlimRegretRepair) {
        int k = 2;

        while (!removed_stores.empty()) {
            struct RegretCandidate {
                int store;
                int route_idx;
                int pos;
                int regret;
            };

            std::vector<RegretCandidate> candidates;

            for (int store : removed_stores) {
                std::vector<std::tuple<int, int, int>> feasible_positions;  // (route, pos, cost)

                int required_ready = inst->getReadyTimes()[store];
                int required_due   = inst->getDueTimes()[store];

                // Evaluate insert positions across all routes
                for (size_t t = 0; t < inst->getRoutes().size(); ++t) {
                    auto& r = inst->getRoutes()[t].getRoute();

                    for (size_t s = 0; s <= r.size(); ++s) {
                        // Guard against early or late time window violations
                        if (s < r.size()) {
                            int neighbor_ready = inst->getReadyTimes()[r[s]];
                            int neighbor_due   = inst->getDueTimes()[r[s]];
                            if (required_due < neighbor_due || required_ready < neighbor_ready) continue;
                        }

                        // Calculate insertion cost proxy (simple distance)
                        int cost = 0;
                        if (s == 0)
                            cost = inst->getDistance(0, store);
                        else if (s == r.size())
                            cost = inst->getDistance(r.back(), store);
                        else
                            cost = inst->getDistance(r[s - 1], store) + inst->getDistance(store, r[s]);

                        feasible_positions.emplace_back(t, s, cost);
                    }
                }

                if ((int)feasible_positions.size() >= k) {
                    // Sort and compute k-regret
                    std::sort(feasible_positions.begin(), feasible_positions.end(),
                              [](auto& a, auto& b) { return std::get<2>(a) < std::get<2>(b); });

                    int regret = std::get<2>(feasible_positions[k - 1]) - std::get<2>(feasible_positions[0]);
                    int route_idx = std::get<0>(feasible_positions[0]);
                    int pos       = std::get<1>(feasible_positions[0]);

                    candidates.push_back({store, route_idx, pos, regret});
                }
            }

            if (!candidates.empty()) {
                // Pick the highest regret candidate to insert first
                std::sort(candidates.begin(), candidates.end(),
                          [](auto& a, auto& b) { return a.regret > b.regret; });

                auto chosen = candidates.front();
                auto& routeObj = inst->getRoutes()[chosen.route_idx];

                if (routeObj.addStore(chosen.store, chosen.pos) != 0) {
                    std::cerr << "[kSlimRegretRepair] Failed to insert store " << chosen.store << "\n";
                    exit(-1);
                }

                removed_stores.erase(std::remove(removed_stores.begin(),
                                                 removed_stores.end(),
                                                 chosen.store),
                                     removed_stores.end());
            } else {
                // Fallback: insert last store randomly
                int store = removed_stores.back();
                removed_stores.pop_back();

                int route_idx = rand() % inst->getRoutes().size();
                auto& routeObj = inst->getRoutes()[route_idx];
                int pos = rand() % (routeObj.size() + 1);

                if (routeObj.addStore(store, pos) != 0) {
                    std::cerr << "[kSlimRegretRepair] Fallback failed for store " << store << "\n";
                    exit(-1);
                }
            }
        }
    }


    if (useProximityReadyRepair == true) {
    
        std::random_device rd;
        std::mt19937 g(rd());
        std::shuffle(removed_stores.begin(), removed_stores.end(), g);
        
        
            
        while (!removed_stores.empty()) {
            
            struct ProximityCandidate {
                int store;
                int route_idx;
                int pos;
                int dist;
                 
                ProximityCandidate(int s, int r_idx, int p, int d): store(s), route_idx(r_idx), pos(p), dist(d) {}
            };
            
            int store = removed_stores.front();
            removed_stores.erase(removed_stores.begin());

            std::vector<ProximityCandidate> candidates;

            int required_ready = inst->getReadyTimes()[store];

            for (size_t t = 0; t < inst->getRoutes().size(); ++t) {
                auto& r = inst->getRoutes()[t].getRoute();

                for (size_t s = 0; s <= r.size(); ++s) {
                    // Insert feasibility check: any prior stop with due_time > store.ready_time
                    bool feasible = false;
                    for (size_t i = 0; i < s; ++i) {
                        if (inst->getDueTimes()[r[i]] > required_ready) {
                            feasible = true;
                            break;
                        }
                    }
                    if (!feasible) continue;

                    // Distance proxy
                    int cost = (s == 0) ? inst->getDistance(0, store)
                              : (s == r.size()) ? inst->getDistance(r.back(), store)
                              : inst->getDistance(r[s - 1], store) + inst->getDistance(store, r[s]);

                    candidates.push_back(ProximityCandidate(store, t, s, cost));
                }
            }

            // Sort candidates for this store
            std::sort(candidates.begin(), candidates.end(),
                      [](auto& a, auto& b) { return a.dist < b.dist; });

            if (!candidates.empty()) {
                auto chosen = candidates.front();
                auto& routeObj = inst->getRoutes()[chosen.route_idx];

                if (routeObj.addStore(chosen.store, chosen.pos) != 0) {
                    std::cerr << "[ProximityReadyRepair] Failed to insert store " << chosen.store << "\n";
                    exit(-1);
                }
            } else {
                // Fallback: insert randomly
                int route_idx = rand() % inst->getRoutes().size();
                auto& routeObj = inst->getRoutes()[route_idx];
                int pos = rand() % (routeObj.size() + 1);

                if (routeObj.addStore(store, pos) != 0) {
                    std::cerr << "[ProximityReadyRepair] Fallback failed for store " << store << "\n";
                    exit(-1);
                }
            }
        }
    }
  
  
    return repair_method;
}
