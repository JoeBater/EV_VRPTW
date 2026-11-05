#pragma once

#include <fstream>
#include <sstream>
#include <unordered_map>
#include <vector>
#include <string>
#include <iostream>

using namespace std;

// One ~100 m piece of an MST edge's terrain profile (from dataprep edge_profiles/)
struct EdgeSegment {
    double length;  // metres
    double dz;      // metres climbed (negative = descent)
};

struct pair_hash {
    std::size_t operator()(const std::pair<int, int>& p) const {
        return std::hash<int>{}(p.first) ^ (std::hash<int>{}(p.second) << 1);
    }
};

class MST
{

public:
    MST();
    MST(string filename);
    ~MST();

    void setName(string n);
    string getName();
    
    void clear();
    void readSpanningTree(string filename);
    std::vector<int> getSteps(int from, int to) const;

    // Terrain profile along each MST edge, both directions. Without one, the
    // solver falls back to a single uniform slope between the two sites.
    void readEdgeProfiles(string filename);
    const std::vector<EdgeSegment>* getSegments(int from, int to) const;  // nullptr if none
    void flattenProfiles();
    size_t profileCount() const { return profiles.size() / 2; }

private:
    string name;
    std::unordered_map<std::pair<int, int>, std::vector<int>, pair_hash> steps;
    std::unordered_map<std::pair<int, int>, std::vector<EdgeSegment>, pair_hash> profiles;
};

