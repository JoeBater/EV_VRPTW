#include "SubStep.hpp"

SubStep::SubStep()
{
}

SubStep::SubStep(int s, int e, std::vector<pair<float, float>> lls, std::vector<double> es, std::vector<double> ds, std::vector<double> ss)
{
    start = s;
    end = e;
    
    latlongs = lls;
    elevations = es;
    distances = ds;
    slopes = ss;
}

SubStep::~SubStep()
{
}

