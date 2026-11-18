#pragma once

#include <vector>
#include <string>

#include "Instance.hpp"
#include "Route.hpp"
#include "SolutionScore.hpp"
#include "LNS.hpp"

using namespace::std;

class exp1
{
public:
    // mode: "1a" terrain only, "1b" weather only, "1c" terrain and weather
    // only: benchmark names to re-simulate (empty = all)
    // detailed: also write the per-minute route logs (large: ~100 MB per city per benchmark)
    exp1(const std::string& mode, const std::vector<std::string>& only = {}, bool detailed = false);
    ~exp1();

};

