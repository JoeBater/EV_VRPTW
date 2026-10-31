#pragma once

#include <vector>

using namespace std;

class SubStep
{
public:
    SubStep();
    SubStep(int s, int e, std::vector<pair<float, float>> lls, std::vector<double> es, std::vector<double> ds, std::vector<double> ss);
    ~SubStep();

private:
    int start;
    int end;
    
    std::vector<std::pair<float, float>> latlongs;
    std::vector<double> elevations;
    std::vector<double> distances;
    std::vector<double> slopes;
};

