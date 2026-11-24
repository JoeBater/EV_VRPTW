#pragma once

#include <vector>
#include <string>

#include "Instance.hpp"
#include "Route.hpp"
#include "SolutionScore.hpp"
#include "LNS.hpp"

using namespace::std;

class exp0
{
public:
    // only: benchmark names to solve (empty = all); iterations: LNS iterations per benchmark
    // completeGraph: exp0a - solve on the complete (Euclidean) graph instead of the MST
    // randomStart: ignore published and previous-size solutions (to test the search itself)
    exp0(const std::vector<std::string>& only = {}, int iterations = 250000, bool completeGraph = false,
         bool randomStart = false);
    ~exp0();

};
