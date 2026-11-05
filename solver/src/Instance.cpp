#include "Instance.hpp"

Instance::Instance(string instanceFile, string spanningTreeFile, string weatherFile, int vc, int nr,
                   string edgeProfileFile)
{
    setVehicleCapacity(vc);
    readInstanceFile(instanceFile);
    readSpanningTree(spanningTreeFile);
    if (!edgeProfileFile.empty()) {
        readEdgeProfiles(edgeProfileFile);
    }
    readWeatherFile(weatherFile);
    //displayWeatherData();
    
    if (nr == 0) {
        nr = estimateNumRoutes();
    }
    initializeRoutesCapacityConstrained(nr);
    //heuristicallySeedRoutes(estimateNumRoutes());
}

Instance::~Instance()
{
    instanceData.clear();
}

int Instance::getInstanceDataSize() {
    return instanceData.size();
}

void Instance::setVerbosity(int v) {
    VERBOSE = v;
}

int Instance::getVerbosity() {
    return VERBOSE;
}

void Instance::setDebug(int d) {
    DEBUG = d;
}

int Instance::getDebug() {
    return DEBUG;
}

double Instance::extractScaleNumber(const std::string& filename) {
    std::stringstream ss(filename);
    std::string segment;
    std::vector<std::string> parts;

    // Split by '_'
    while (std::getline(ss, segment, '_')) {
        parts.push_back(segment);
    }

    if (parts.size() > 1) {
        try {
            return std::stod(parts[1]);  // Convert "2.5" to double
        } catch (const std::invalid_argument& e) {
            std::cerr << "Could not convert to double: " << parts[1] << std::endl;
        } catch (const std::out_of_range& e) {
            std::cerr << "Number out of range: " << parts[1] << std::endl;
        }
    }

    return 0.0;  // Fallback if format doesn’t match
}

// Function to read a CSV file and store the data in a 2D vector
void Instance::readInstanceFile(string filename) {
    
    instanceData.clear();

    std::ifstream file(filename);
    std::string line;
    
    if (!file.is_open()) {
        std::cerr << "Error opening file! " << filename << std::endl;
        return;
    }

    
    
    int lineCount = 0;
    while (std::getline(file, line)) {
        lineCount++;
        if (lineCount <= 1) continue; // Skip header lines

        std::stringstream ss(line);
        StoreData data;
        std::string token;
        int columnIndex = 0;
        

        while (std::getline(ss, token, ',')) {
            
            // Trim spaces
            token.erase(0, token.find_first_not_of(" \t\r\n"));
            token.erase(token.find_last_not_of(" \t\r\n") + 1);
            
            //std::cout << "Col " << columnIndex << ": " << token << std::endl;
            
            switch (columnIndex) {
                case 0: data.id = std::stoi(token)-1; break;
                case 3: data.demand = std::stoi(token); break;
                case 4: data.ready_time = std::stoi(token); break;
                case 5: data.due_time = std::stoi(token); break;
                case 6: data.service_time = std::stoi(token); break;
                case 7: data.x = std::stoi(token); break;
                case 8: data.y = std::stoi(token); break;
                case 11: data.elevation = std::stod(token); break;
                
            }
            columnIndex++;
        }

        instanceData.push_back(data);
    }

    file.close();
    
    //displayWeatherData();
    if (VERBOSE) {
        std::cout << "Instance Data: " << instanceData.size() << " rows" << endl;
    }

    generateDistanceMatrix();
    generateSlopeMatrix();
    
    //displayInstanceData();
    setDemands();
    setReadyTimes();
    setDueTimes();
    setDueTimes();
    setServiceTimes();
};

void Instance::readSpanningTree(string filename) {
    if (VERBOSE) {
        cout << "reading spanning tree <" + filename + ">\n";
    }
    spanning_tree.clear();
    spanning_tree.readSpanningTree(filename);
}

void Instance::readEdgeProfiles(string filename) {
    if (VERBOSE) {
        cout << "reading edge profiles <" + filename + ">\n";
    }
    spanning_tree.readEdgeProfiles(filename);
}

MST Instance::getSpanningTree() {
    return spanning_tree;
}




void Instance::readWeatherFile(string filename) {

    // Clear previous data
    weatherData.clear(); 

    std::ifstream file(filename);
    std::string line;
    
    if (!file.is_open()) {
        std::cerr << "Error opening file! " << filename << std::endl;
        return;
    }

    int lineCount = 0;
    while (std::getline(file, line)) {
        lineCount++;
        if (lineCount <= 1) continue; // Skip header lines

        std::stringstream ss(line);
        WeatherData data;
        std::string token;
        int columnIndex = 0;
        

        while (std::getline(ss, token, ',')) {
            
            // Trim spaces
            token.erase(0, token.find_first_not_of(" \t\r\n"));
            token.erase(token.find_last_not_of(" \t\r\n") + 1);
                
            switch (columnIndex) {
                case 0: data.month = std::stoi(token); break;
                case 1: data.day = std::stoi(token); break;
                case 2: data.hour = std::stoi(token); break;  
                case 3: data.temp_C = std::stod(token); break;
                case 5: data.rain_mm = std::stod(token); break;
                
            }
            columnIndex++;
        }

        weatherData.push_back(data);
//        //if (DEBUG) {
//        std::cout << "Month: " << data.month << " Day: " << data.day << " Hour: " << data.hour << " Temperature: " << data.temp_C << " Rain: " << data.rain_mm << endl;
//        //}
    }

    file.close();
    
    //displayWeatherData();
    if (VERBOSE) {
        std::cout << "Weather Data: " << weatherData.size() << " rows" << endl;
    }
};

vector<Route>& Instance::getRoutes() {
    return routes;
}

const vector<Route>& Instance::getRoutes() const {
    return routes;
}

void Instance::calculateRoutes() {
    
    // Calculate each route
    for (auto& route : routes) {
        //route.calculateRoute(distanceMatrix, slopeMatrix, ready_times, due_times, service_times, vehicle_capacity, demands, temperatures, rainfall);
        route.simulateRouteTimelineMST(distanceMatrix, slopeMatrix, spanning_tree, ready_times, due_times, service_times, vehicle_capacity, demands, temperatures, rainfall);
    }
}

void Instance::setRoutes(std::vector<Route> rtes) {
    
    routes.clear();
    for (auto r : rtes) {
        routes.push_back(r);
    }
}

const std::vector<std::vector<int>>& Instance::getDistanceMatrix() const { return distanceMatrix; }
const std::vector<std::vector<double>>& Instance::getSlopeMatrix() const { return slopeMatrix; }
const std::vector<int>& Instance::getReadyTimes() const { return ready_times; }
const std::vector<double>& Instance::getTemperatures() const { return temperatures; }
const std::vector<double>& Instance::getRainfall() const { return rainfall; }

void Instance::getTodaysWeather(int mm, int dd) {
    
    temperatures = std::vector<double>(24, 0);
    rainfall = std::vector<double>(24, 0);
    
    double sum_temp = 0.0;
    double sum_rain = 0.0;
    
    bool dataFound = false;
    for (const auto& entry : weatherData) {
        if (entry.month == mm && entry.day == dd) {
            int h = entry.hour;
            if (h >= 0 && h < 24) {
                temperatures[h] = entry.temp_C;
                sum_temp += temperatures[h];
                rainfall[h] = entry.rain_mm;
                sum_rain += rainfall[h];
                dataFound = true;
            }
        }
    }
    if (!dataFound) {
        cout << "No weather data found for " << mm << "/" << dd << endl;
    } else {
        cout << "Average temperature: " << (sum_temp / 24.0) << endl;
        cout << "Average rainfall: " << (sum_rain / 24.0) << endl;
    }
}

void Instance::getMonthAverageWeather(int mm) {
    
    temperatures = std::vector<double>(24, 0);
    rainfall = std::vector<double>(24, 0);
    
    double sum_temp = 0.0;
    double sum_rain = 0.0;
    
    double days_count = 0.0;
    
    bool dataFound = false;
    for (const auto& entry : weatherData) {
        if (entry.month == mm) {
            int h = entry.hour;
            if (h >= 0 && h < 24) {
                temperatures[h] += entry.temp_C;
                sum_temp += temperatures[h];
                rainfall[h] += entry.rain_mm;
                sum_rain += rainfall[h];
                dataFound = true;
            }
            days_count += 1.0;
        }
    }
    
    for (size_t t = 0; t < temperatures.size(); t++) {
        temperatures[t] = temperatures[t] / 24.0;
        rainfall[t] = rainfall[t] / 24.0;
        
        cerr << t << " temperature " << temperatures[t] << " rainfall " << rainfall[t] << "\n";
    }
    
    if (!dataFound) {
        cout << "No weather data found for " << mm << endl;
    } else {
        cout << "Average temperature: " << (sum_temp / days_count) << endl;
        cout << "Average rainfall: " << (sum_rain / days_count) << endl;
    }
}

void Instance::flattenWeather(double tt) {

    for (size_t t = 0; t < temperatures.size(); t++) {
        temperatures[t] = tt;
        rainfall[t] = 0.0;
    }
}

double Instance::getElevation(int s1) {
    return instanceData[s1].elevation;
}

int Instance::getTotalDistance() {
    int total_d = 0;
    for (size_t t = 0; t < routes.size(); t++) {
        total_d += routes[t].getDistance();
    }
    return total_d;
}

int Instance::getTotalLate() {
    int total_late = 0;
    for (size_t t = 0; t < routes.size(); t++) {
        total_late += routes[t].getTotalLate();
    }
    return total_late;
}

int Instance::getTotalTardiness() {
    int total_tardiness = 0;
    for (const auto& route : routes) {
        for (int minutes : route.getLatenessPerStop()) {
            total_tardiness += minutes;
        }
    }
    return total_tardiness;
}

int Instance::getTotalOverload() {
    int total_overload = 0;
    for (size_t t = 0; t < routes.size(); t++) {
        total_overload += std::max(routes[t].getTotalLoad() - vehicle_capacity, 0);
    }
    return total_overload;
}

void Instance::displayInstanceData() {
    if (VERBOSE) {
        std::cout << "Instance Data " << instanceData.size() << " rows" << endl;
        for (const auto& data : instanceData) {
            cout << "id: " << data.id << ", X: " << data.x << ", Y: " << data.y << ", Elevation: ";
            cout << data.elevation << ", ready time " << data.ready_time << ", due time " << data.due_time << ", service time " << data.service_time << endl;
        }
        cout << endl;
    }
};

vector<StoreData> Instance::getInstanceData() const {
    return instanceData;
}

vector<vector<int>> Instance::getDistanceMatrix() {
    return distanceMatrix;
}

int Instance::getDistance(int s1, int s2) {
    return distanceMatrix[s1][s2];
}

vector<vector<double>> Instance::getSlopeMatrix() {
    return slopeMatrix;
}

double Instance::getSlope(int s1, int s2) {
    return slopeMatrix[s1][s2];
}

void Instance::setVehicleCapacity(int c) {
    vehicle_capacity = c;
}

int Instance::getVehicleCapacity() {
    return vehicle_capacity;
}

void Instance::setDemands() {
    demands.clear();
    
    for (size_t t = 0; t < instanceData.size(); t++) {
        demands.push_back(instanceData[t].demand);
    }
}

vector<int> Instance::getDemands() {
    return demands;
}

void Instance::setReadyTimes() {
    ready_times.clear();
    
    for (size_t t = 0; t < instanceData.size(); t++) {
        ready_times.push_back(instanceData[t].ready_time);
    }
}

vector<int> Instance::getReadyTimes() {
    return ready_times;
}

void Instance::setDueTimes() {
    due_times.clear();
    
    for (size_t t = 0; t < instanceData.size(); t++) {
        due_times.push_back(instanceData[t].due_time);
    }
}

vector<int> Instance::getDueTimes() {
    return due_times;
}

void Instance::setServiceTimes() {
    service_times.clear();
    
    for (size_t t = 0; t < instanceData.size(); t++) {
        service_times.push_back(instanceData[t].service_time);
    }
}

vector<int> Instance::getServiceTimes() {
    return service_times;
}

int Instance::getServiceTimeSite(int s) {
    return service_times[s];
}

void Instance::setLatePenalties() {
    late_penalties.clear();
    
    for (size_t t = 0; t < instanceData.size(); t++) {
       late_penalties.push_back(LATE_PENALTY);
    }
}

vector<int> Instance::getLatePenalties() {
    return late_penalties;
}

void Instance::displayWeatherData() {
    if (VERBOSE) {
        std::cout << "Weather Data: " << weatherData.size() << " rows" << endl;
        for (const auto& data : weatherData) {
            std::cout << "Month: " << data.month << ", Day: " << data.day << ", Hour: " << data.hour 
                        << ", Dry Bulb Temp (C): " << data.temp_C << ", Rainfall (mm/hr):" << data.rain_mm << endl;
        }
        std::cout << endl;
    }
};

// function to generate distance matrix from the data
void Instance::generateDistanceMatrix() {
    int n = instanceData.size();

    if (n == 0) {
        cerr << "No data available to generate distance matrix." << endl;
        return;
    }
    // re-allocate the distance matrix to the size of the data
    distanceMatrix.clear(); // Clear the previous distance matrix
    distanceMatrix.resize(n, vector<int>(n, 0));

    // Calculate the distance between each pair of points
    for (int i = 0; i < n; ++i) {
        for (int j = 0; j < n; ++j) {
            if (i != j) {
                int dx = instanceData[i].x - instanceData[j].x;
                int dy = instanceData[i].y - instanceData[j].y;
                distanceMatrix[i][j] = ceil(sqrt(dx * dx + dy * dy));
            } else {
                distanceMatrix[i][j] = 0;
            }
        }
    }

    //return distanceMatrix;
};

void Instance::generateSlopeMatrix() {
    int n = instanceData.size();

    if (n == 0) {
        cerr << "No data available to generate slope matrix." << endl;
        return;
    }
    // re-allocate the distance matrix to the size of the data
    slopeMatrix.clear(); // Clear the previous distance matrix
    slopeMatrix.resize(n, vector<double>(n, 0.0));

    // Calculate the distance between each pair of points
    for (int i = 0; i < n; ++i) {
        for (int j = 0; j < n; ++j) {
            if (i != j) {
                double ei = instanceData[i].elevation;
                double ej = instanceData[j].elevation;
                double dist = distanceMatrix[i][j];
                slopeMatrix[i][j] = (dist > 0.001) ? ((ej - ei) / dist) : 0.0;
            }
        }
    }

    //return slopeMatrix;
};

void Instance::flattenCity() {
    
    int n = instanceData.size();
    for (int i = 0; i < n; ++i) {
        for (int j = 0; j < n; ++j) {
            slopeMatrix[i][j] = 0.0;
        }
    }
    spanning_tree.flattenProfiles();
}

void Instance::displayDistanceMatrix() {
    if (VERBOSE) {
        std::cout << "Distance Matrix " << distanceMatrix.size() << " rows" << endl;
        for (const auto& row : distanceMatrix) {
            for (const auto& item : row) {
                std::cout << item << " ";
            }
            std::cout << endl;
        }
        std::cout << endl;
    }
};

int Instance::estimateNumRoutes() {
    
    // estimate the number of routes required
    int total_demand = 0;
    for (size_t t = 0; t < instanceData.size(); t++) {
        total_demand += instanceData[t].demand;
    }
    
    float num_routes = std::ceil(total_demand / vehicle_capacity) + 2;
    
    if (VERBOSE) {
        cout << "Estimating as " << num_routes << " routes to complete" << endl;
    }
    return (int)num_routes;
}

bool Instance::isTimeWindowFeasible(const std::vector<int>& rte) const {
    int time = 0;
    for (size_t i = 0; i < rte.size(); ++i) {
        int node = rte[i];
        int travel = (i == 0) ? distanceMatrix[0][node] : distanceMatrix[rte[i - 1]][node];
        time += travel;

        time = std::max(time, ready_times[node]);
        if (time > due_times[node]) return false;

        time += service_times[node];
    }
    return true;
}


void Instance::initializeRoutes(int nr) {
    
    routes.clear();
    if (VERBOSE) {
        cout << "setting " << nr << " routes\n";
    }
    for (int r = 0; r < nr; r++) {
        routes.push_back(Route());
        routes[r].setRouteNumber(r);
    }

    // Create a list of all store indices (excluding depot)
    std::vector<int> store_indices;
    for (size_t s = 1; s < instanceData.size(); s++) {
        store_indices.push_back(s);
    }

    // Shuffle the store indices to randomize assignment
    std::random_device rd;
    std::mt19937 g(rd());
    std::shuffle(store_indices.begin(), store_indices.end(), g);

    // Assign stores round-robin to routes
    for (size_t i = 0; i < store_indices.size(); ++i) {
        int route_idx = i % nr;
        routes[route_idx].addStore(store_indices[i]);
    }

    // Calculate each route
    getTodaysWeather(1, 1);
    for (auto& route : routes) {
        //route.calculateRoute(distanceMatrix, slopeMatrix, ready_times, due_times, service_times, vehicle_capacity, demands, temperatures, rainfall);
        route.simulateRouteTimelineMST(distanceMatrix, slopeMatrix, spanning_tree, ready_times, due_times, service_times, vehicle_capacity, demands, temperatures, rainfall);
        if (VERBOSE) {
            route.display();
        }
    }
    
    
    double avg_route_dist = 0.0;
    for (auto& route : routes) {
        avg_route_dist += route.getDistance();
    }
    avg_route_dist /= (double)routes.size();
    cout << "avg route distance " << avg_route_dist << endl;
    
    double avg_final_soc = 0.0;
    for (auto& route : routes) {
        avg_final_soc += route.getFinalSoC();
    }
    avg_final_soc /= (double)routes.size();
    cout << "avg final SoC " << avg_final_soc << endl;
    
    
}


void Instance::initializeRoutesCapacityConstrained(int nr) {
    
    getTodaysWeather(1, 1);
    cerr << "capacity constrained route init" << endl;
    routes.clear();
    if (VERBOSE) {
        cout << "setting " << nr << " routes\n";
    }
    for (int r = 0; r < nr; r++) {
        routes.push_back(Route());
        routes[r].setRouteNumber(r);
    }

    // Create a list of all store indices (excluding depot)
    std::vector<int> store_indices;
    for (size_t s = 1; s < instanceData.size(); s++) {
        store_indices.push_back(s);
    }

    // Shuffle the store indices to randomize assignment
    std::random_device rd;
    std::mt19937 g(rd());
    std::shuffle(store_indices.begin(), store_indices.end(), g);

    // Assign stores round-robin to routes
    int stores_assigned = 0;
    for (size_t i = 0; i < store_indices.size(); ++i) {
        for (size_t r = 0; r < routes.size(); r++) {
            int route_idx = i % nr;
            if (routes[route_idx].getTotalLoad() < routes[route_idx].getCapacity()) {
                routes[route_idx].addStore(store_indices[i]);
                stores_assigned += 1;
                break;
            }
        }
    }
    if (stores_assigned < (int)store_indices.size()) {
        cerr << "[INIT CAPACITY CONSTRAINED] - not all stores assigned to routes!" << endl;
    }
    cerr << "set routes " << endl;

    // Calculate each route
    for (auto& route : routes) {
        //route.calculateRoute(distanceMatrix, slopeMatrix, ready_times, due_times, service_times, vehicle_capacity, demands, temperatures, rainfall);
        route.simulateRouteTimelineMST(distanceMatrix, slopeMatrix, spanning_tree, ready_times, due_times, service_times, vehicle_capacity, demands, temperatures, rainfall);
        if (VERBOSE) {
            route.display();
        }
    }
    
    
    double avg_route_dist = 0.0;
    for (auto& route : routes) {
        avg_route_dist += route.getDistance();
    }
    avg_route_dist /= (double)routes.size();
    cout << "avg route distance " << avg_route_dist << endl;
    
    double avg_final_soc = 0.0;
    for (auto& route : routes) {
        avg_final_soc += route.getFinalSoC();
    }
    avg_final_soc /= (double)routes.size();
    cout << "avg final SoC " << avg_final_soc << endl;
}

bool Instance::checkAllStoresVisitedOnce() {
    std::unordered_set<int> seen;
    for (auto& route : routes) {
        for (int s = 0; s < route.size(); s++) {
            int store = route.getRoute(s);
            if (seen.count(store)) {
                std::cerr << "Duplicate store found: " << store << " in route " << "\n";
                return false;
            }
            seen.insert(store);
        }
    }
    std::size_t expected = getInstanceDataSize() - 1;
    return seen.size() == expected;
}

void Instance::heuristicallySeedRoutes(int nr) {
    
    routes.clear();
    if (VERBOSE) {
        cout << "heuristically seeding " << nr << " routes\n";
    }
    for (int r = 0; r < nr; r++) {
        routes.push_back(Route());
        routes[r].setRouteNumber(r);
    }

    // Create a list of all store indices (excluding depot) ordered by ready times
    std::vector<std::pair<int, int>> store_indices;
    for (size_t s = 1; s < instanceData.size(); s++) {
        store_indices.emplace_back(s, ready_times[s]);
    }

    // Sort by ready time
    std::sort(store_indices.begin(), store_indices.end(),
              [](const std::pair<int, int>& a, const std::pair<int, int>& b) {
                  return a.second < b.second;
              });

    // Seed first `nr` routes with the earliest stores
    for (int t = 0; t < nr && t < (int)store_indices.size(); t++) {
        routes[t].addStore(store_indices[t].first);
    }

    // Assign remaining stores to nearest route endpoint
    for (size_t t = nr; t < store_indices.size(); t++) {
        int x = store_indices[t].first;
        int closest_route_index = -1;
        int closest_dist = INT_MAX;

        for (size_t i = 0; i < routes.size(); i++) {
            int y = routes[i].getRoute().back(); // last store in route
            if (distanceMatrix[x][y] < closest_dist) {
                closest_dist = distanceMatrix[x][y];
                closest_route_index = i;
            }
        }

        if (closest_route_index != -1) {
            routes[closest_route_index].addStore(x);
        } else {
            std::cerr << "Error: could not assign store " << x << " to any route!\n";
        }
    }
    assert(checkAllStoresVisitedOnce());

    // Calculate each route
    getTodaysWeather(1, 1);
    for (auto& route : routes) {
        //route.calculateRoute(distanceMatrix, slopeMatrix, ready_times, due_times, service_times, vehicle_capacity, demands, temperatures, rainfall);
        route.simulateRouteTimelineMST(distanceMatrix, slopeMatrix, spanning_tree, ready_times, due_times, service_times, vehicle_capacity, demands, temperatures, rainfall);
        if (VERBOSE) {
            route.display();
        }
    }
    
    
    double avg_route_dist = 0.0;
    for (auto& route : routes) {
        avg_route_dist += route.getDistance();
    }
    avg_route_dist /= (double)routes.size();
    cout << "avg route distance " << avg_route_dist << endl;
    
    double avg_final_soc = 0.0;
    for (auto& route : routes) {
        avg_final_soc += route.getFinalSoC();
    }
    avg_final_soc /= (double)routes.size();
    cout << "avg final SoC " << avg_final_soc << endl;
    
    
}

void Instance::displayRoutes() {
    
    for (size_t r = 0; r < routes.size(); r++) {
        routes[r].display();
    }
}

void Instance::writeRoutesToFile(string filename) {
    
    std::ofstream file(filename);
    std::string line;
    
    if (!file.is_open()) {
        std::cerr << "Error opening file! " << filename << std::endl;
        return;
    }
    
    for (size_t r = 0; r < routes.size(); r++) {
        
        vector<int> temp = routes[r].getRoute();
        file << r << " :";
        for (size_t t = 0; t < temp.size(); t++) {
            file << " " << temp[t];
        }
        file << endl;
    }
    
    file.close();
}

void Instance::readRoutesFromFile(std::string filename) {
    
    std::ifstream file(filename);
    std::string line;

    // Clear any existing routes
    routes.clear();

    if (!file.is_open()) {
        std::cerr << "Error opening file: " << filename << std::endl;
        return;
    }

    while (std::getline(file, line)) {
        std::stringstream ss(line);
        std::string token;

        int route_id;
        char colon;

        // Read the route ID and the colon
        ss >> route_id >> colon;
        if (colon != ':') {
            std::cerr << "Malformed route line: " << line << std::endl;
            continue;
        }

        std::vector<int> new_route;

        // Read the route sequence
        int node;
        while (ss >> node) {
            new_route.push_back(node);
        }

        // Construct the Route object and add to instance
        Route route(new_route);  // assuming Route has a constructor that takes vector<int>
        routes.push_back(route);
    }

    file.close();
}

void Instance::logDetailedRoutes(const std::string filename) {
    std::fstream f(filename, std::ios::out | std::ios::trunc);
    if (!f) {
        std::cerr << "could not open file <" << filename << "> !!!" << std::endl;
        return;
    }

    f << "route_id,minute,soc_used,soc_regen,cumulative_soc,activity,timeliness,speed,dist_per_minute,step_distance,slope,temperature,precipitation,load,leg_location_id,step_location_id\n";
    f << std::fixed << std::setprecision(6);

    for (const auto& route : routes) {
        vector<SoCEntry> detail = route.getSoCDetail();
        for (size_t t = 0; t < detail.size(); t++) {
 
            f << detail[t].route_id << ","
              << detail[t].minute << ","
              << detail[t].soc_used << ","
              << detail[t].soc_regen << ","
              << detail[t].cumulative_soc << ","
              << detail[t].activity << ","
              << detail[t].timeliness << ","
              << detail[t].speed << ","
              << detail[t].dist_per_minute << ","
              << detail[t].step_distance << ","
              << detail[t].slope << ","
              << detail[t].temperature << ","
              << detail[t].precipitation << ","
              << detail[t].loading << ","
              << detail[t].leg_location_id << ","
              << detail[t].step_location_id << "\n";
        }
    }

    f.close();
}


