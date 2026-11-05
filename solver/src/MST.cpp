#include "MST.hpp"

#include <algorithm>

MST::MST()
{
}

MST::MST(string filename)
{
    readSpanningTree(filename);
}

void MST::readSpanningTree(string filename) 
{    
    steps.clear();
    
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Error: Unable to open file <" << filename << ">!" << std::endl;
        return;
    }

    std::string line;
    while (std::getline(file, line)) {
        // Skip empty lines or comment lines (optional)
        if (line.empty() || line[0] == '#') continue;

        //std::cout << line << endl;

        // Split line at colon
        size_t colon_pos = line.find(":");
        if (colon_pos == std::string::npos) {
            std::cerr << "Malformed line: " << line << std::endl;
            continue;
        }

        std::string fromto = line.substr(0, colon_pos);
        std::string steps_part = line.substr(colon_pos + 1);

        std::istringstream fromto_stream(fromto);
        int from, to;
        fromto_stream >> from >> to;

        std::vector<int> intermediate_steps;
        std::istringstream steps_stream(steps_part);
        int node;
        while (steps_stream >> node) {
            intermediate_steps.push_back(node);
        }

        steps[{from, to}] = intermediate_steps;
    }

    file.close();
}

MST::~MST()
{
}

void MST::clear() {
    steps.clear();
    profiles.clear();
}

void MST::setName(string n) {
    name = n;
}

string MST::getName() {
    return name;
}

std::vector<int> MST::getSteps(int ss, int ff) const {
    
    auto it = steps.find({ss, ff});
    if (it != steps.end()) {
        return it->second;
    } 
    
    return {}; // implies direct traversal (i → j)
}

void MST::readEdgeProfiles(string filename)
{
    profiles.clear();

    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Error: Unable to open file <" << filename << ">!" << std::endl;
        return;
    }

    // start,end,length,dz - one row per segment, in order from start to end
    std::string line;
    std::getline(file, line);  // header
    while (std::getline(file, line)) {
        if (line.empty()) continue;
        std::replace(line.begin(), line.end(), ',', ' ');
        std::istringstream row(line);
        int from, to;
        EdgeSegment segment;
        if (!(row >> from >> to >> segment.length >> segment.dz)) {
            std::cerr << "Malformed line: " << line << std::endl;
            continue;
        }
        profiles[{from, to}].push_back(segment);
    }
    file.close();

    // the reverse direction walks the same segments backwards, downhill for uphill
    std::vector<std::pair<int, int>> forward;
    for (const auto& [edge, _] : profiles) forward.push_back(edge);
    for (const auto& [from, to] : forward) {
        std::vector<EdgeSegment> reverse(profiles[{from, to}].rbegin(), profiles[{from, to}].rend());
        for (auto& segment : reverse) segment.dz = -segment.dz;
        profiles[{to, from}] = reverse;
    }
}

const std::vector<EdgeSegment>* MST::getSegments(int from, int to) const
{
    auto it = profiles.find({from, to});
    return it != profiles.end() ? &it->second : nullptr;
}

void MST::flattenProfiles()
{
    for (auto& [_, segments] : profiles)
        for (auto& segment : segments) segment.dz = 0.0;
}
