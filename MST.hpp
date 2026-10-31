#pragma once

#include <fstream>
#include <sstream>
#include <unordered_map>
#include <vector>
#include <string>
#include <iostream>

using namespace std;

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

private:
    string name;
    std::unordered_map<std::pair<int, int>, std::vector<int>, pair_hash> steps;
};

