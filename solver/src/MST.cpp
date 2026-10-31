#include "MST.hpp"

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
