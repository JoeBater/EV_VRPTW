#include "MST.hpp"

#include <algorithm>

MST::MST()
{
}

MST::MST(string filename)
{
    readSpanningTree(filename);
}

// Two formats, told apart line by line:
//   "from to: s1 s2 ..."  one precomputed path per pair (Solomon *_short_paths.txt)
//   "u v"                 one MST edge per line (*_mst.txt); paths are walked on the tree
//                         in getSteps, since listing every pair of a 1000-site tree
//                         would take ~400 MB
void MST::readSpanningTree(string filename)
{
    steps.clear();
    parent.clear();
    depth.clear();
    std::vector<std::pair<int, int>> edges;

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
            std::istringstream edge(line);
            int u, v;
            if (edge >> u >> v) edges.push_back({u, v});
            else std::cerr << "Malformed line: " << line << std::endl;
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
    if (!edges.empty()) buildTree(edges);
}

// Root the tree at the depot (site 0): parent and depth of every site, by breadth-first search
void MST::buildTree(const std::vector<std::pair<int, int>>& edges)
{
    int n = 0;
    for (const auto& [u, v] : edges) n = std::max(n, std::max(u, v) + 1);
    std::vector<std::vector<int>> adjacent(n);
    for (const auto& [u, v] : edges) {
        adjacent[u].push_back(v);
        adjacent[v].push_back(u);
    }
    parent.assign(n, -1);
    depth.assign(n, -1);
    depth[0] = 0;
    std::vector<int> queue{0};
    for (size_t i = 0; i < queue.size(); i++) {
        int u = queue[i];
        for (int v : adjacent[u]) {
            if (depth[v] < 0) {
                depth[v] = depth[u] + 1;
                parent[v] = u;
                queue.push_back(v);
            }
        }
    }
    if ((int)queue.size() != n || (int)edges.size() != n - 1)
        std::cerr << "Warning: MST edges do not form one tree over " << n << " sites" << std::endl;
}

MST::~MST()
{
}

void MST::clear() {
    steps.clear();
    parent.clear();
    depth.clear();
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

    // tree format: climb from both ends to their lowest common ancestor
    int n = (int)parent.size();
    if (ss != ff && ss >= 0 && ff >= 0 && ss < n && ff < n) {
        std::vector<int> up, down;   // ss .. ancestor, and ff .. ancestor
        int a = ss, b = ff;
        while (depth[a] > depth[b]) { a = parent[a]; up.push_back(a); }
        while (depth[b] > depth[a]) { b = parent[b]; down.push_back(b); }
        while (a != b) {
            a = parent[a]; up.push_back(a);
            b = parent[b]; down.push_back(b);
        }
        // the ancestor ends both lists; drop it from one, then drop ff itself
        if (!down.empty()) down.pop_back();
        else up.pop_back();            // ff is an ancestor of ss: up ends with ff
        up.insert(up.end(), down.rbegin(), down.rend());
        return up;
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
