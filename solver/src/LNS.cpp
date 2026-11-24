#include "LNS.hpp"

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <numeric>

static const double INF = 1e18;

// Adaptive weights (Ropke & Pisinger 2006): scores per use, folded in every SEGMENT iterations
static const int SEGMENT = 100;
static const double REACTION = 0.1;
static const double SCORE_NEW_BEST = 33.0;
static const double SCORE_BETTER = 9.0;
static const double SCORE_ACCEPTED = 13.0;
static const double MIN_WEIGHT = 0.1;

// Simulated annealing: a solution 5% longer than the start is first accepted with probability
// 1/2, and the temperature falls geometrically to 0.2% of that by the last iteration
static const double START_WORSE = 0.05;
static const double END_TEMPERATURE = 0.002;

static const double NOISE = 0.025;  // insertion-cost noise, as a share of the longest distance

LNS::LNS(Instance* instance, string plFile, string vdDir, unsigned seed)
    : inst(instance), rng(seed), progressLogFile(plFile), videoDir(vdDir)
{
    VERBOSE = inst->getVerbosity();
    VIDEO = !videoDir.empty();

    nSites = inst->getInstanceDataSize();
    nCustomers = nSites - 1;
    capacity = inst->getVehicleCapacity();
    ready = inst->getReadyTimes();
    due = inst->getDueTimes();
    service = inst->getServiceTimes();
    demand = inst->getDemands();
    D = inst->getDistanceMatrix();
    T0 = inst->travelMinutes();
    horizon = std::max(1, due[0]);
    for (int i = 0; i < nSites; i++) {
        maxDemand = std::max(maxDemand, (double)demand[i]);
        for (int j = 0; j < nSites; j++) maxDistance = std::max(maxDistance, (double)D[i][j]);
    }

    destroyOps = {{"RandomRemoval"}, {"WorstRemoval"}, {"RelatedRemoval"}, {"RouteRemoval"}, {"LateRemoval"}};
    repairOps = {{"GreedyInsertion"}, {"GreedyInsertion+noise"},
                 {"Regret2Insertion"}, {"Regret2Insertion+noise"},
                 {"Regret3Insertion"}, {"Regret3Insertion+noise"}};
}

LNS::~LNS()
{
}

// ---------------------------------------------------------------------------------------------
// Costs and schedules

// whole minutes, as Route's per-minute simulation takes them
int LNS::legTime(int a, int b, int load) const
{
    double t = T0[a][b];
    if (t <= 0.0) return 0;
    return (int)std::ceil(t / Route::loadSpeedFactor(load) - 1e-9);
}

LNS::RouteCost LNS::evaluate(int r)
{
    Route& route = inst->getRoutes()[r];
    inst->simulate(route);
    RouteCost c;
    c.distance = (int)route.getDistance();
    c.late = route.getTotalLate();
    for (int minutes : route.getLatenessPerStop()) c.tardiness += minutes;
    c.overload = std::max(0, route.getTotalLoad() - capacity);
    c.cost = w_distance * c.distance + w_lateness * c.late + w_tardiness * c.tardiness + w_capacity * c.overload;
    return c;
}

void LNS::buildSchedule(int r)
{
    const vector<int>& s = seq(r);
    Schedule& h = sched[r];
    int m = s.size();
    h.arrive.assign(m + 1, 0);
    h.depart.assign(m, 0);
    h.latest.assign(m + 1, 0);
    h.legLoad.assign(m + 1, 0);
    h.load = 0;
    for (int c : s) h.load += demand[c];
    h.latest[m] = due[0];
    if (m == 0) return;

    // as in Route: the leg into stop k carries everything not yet delivered
    h.legLoad[0] = h.load;
    for (int k = 1; k < m; k++) h.legLoad[k] = h.legLoad[k - 1] - demand[s[k - 1]];

    int t = 0, prev = 0;
    for (int k = 0; k < m; k++) {
        t += legTime(prev, s[k], h.legLoad[k]);
        h.arrive[k] = t;
        t = std::max(t, ready[s[k]]) + service[s[k]];
        h.depart[k] = t;
        prev = s[k];
    }
    h.arrive[m] = t + legTime(prev, 0, 0);

    for (int k = m - 1; k >= 0; k--) {
        int next = (k + 1 < m) ? s[k + 1] : 0;
        h.latest[k] = std::min(due[s[k]], h.latest[k + 1] - legTime(s[k], next, h.legLoad[k + 1]) - service[s[k]]);
    }
}

// Estimated cost of putting store c before position p of route r (p = size: at the end)
double LNS::insertionCost(int c, int r, int p)
{
    const vector<int>& s = seq(r);
    const Schedule& h = sched[r];
    int m = s.size();
    int prev = (p == 0) ? 0 : s[p - 1];
    int next = (p == m) ? 0 : s[p];

    int loadC = ((p == 0) ? h.load : h.legLoad[p - 1] - demand[s[p - 1]]) + demand[c];
    int loadNext = loadC - demand[c];   // 0 back to the depot
    int arriveC = ((p == 0) ? 0 : h.depart[p - 1]) + legTime(prev, c, loadC);
    int departC = std::max(arriveC, ready[c]) + service[c];
    int arriveNext = departC + legTime(c, next, loadNext);

    double penalty = 0.0;
    if (arriveC > due[c]) penalty += w_lateness + w_tardiness * (arriveC - due[c]);
    int push = arriveNext - std::max(h.latest[p], h.arrive[p]);   // delay the rest of the route can't absorb
    if (push > 0) penalty += w_lateness + w_tardiness * push;

    double detour = D[prev][c] + D[c][next] - D[prev][next];
    int overload = std::max(0, h.load + demand[c] - capacity) - std::max(0, h.load - capacity);
    return w_distance * detour + penalty + w_capacity * overload;
}

// Estimated cost of a whole route, without capacity (which the order doesn't change)
double LNS::estimateCost(const vector<int>& s) const
{
    if (s.empty()) return 0.0;
    int load = 0;
    for (int c : s) load += demand[c];
    int t = 0, prev = 0;
    double distance = 0.0, penalty = 0.0;
    for (size_t k = 0; k < s.size(); k++) {
        if (k > 0) load -= demand[s[k - 1]];
        t += legTime(prev, s[k], load);
        distance += D[prev][s[k]];
        if (t > due[s[k]]) penalty += w_lateness + w_tardiness * (t - due[s[k]]);
        t = std::max(t, ready[s[k]]) + service[s[k]];
        prev = s[k];
    }
    t += legTime(prev, 0, 0);
    distance += D[prev][0];
    if (t > due[0]) penalty += w_lateness + w_tardiness * (t - due[0]);
    return w_distance * distance + penalty;
}

void LNS::touch(int r)
{
    if (touched[r]) return;
    touched[r] = 1;
    backups.push_back({r, inst->getRoutes()[r], cost[r], sched[r]});
}

void LNS::locate()
{
    routeOf.assign(nSites, -1);
    posOf.assign(nSites, -1);
    for (int r = 0; r < (int)inst->getRoutes().size(); r++) {
        const vector<int>& s = seq(r);
        for (int p = 0; p < (int)s.size(); p++) {
            routeOf[s[p]] = r;
            posOf[s[p]] = p;
        }
    }
}

int LNS::pick(const vector<Operator>& ops)
{
    double total = 0.0;
    for (const Operator& o : ops) total += o.weight;
    double u = std::uniform_real_distribution<double>(0.0, total)(rng);
    for (size_t i = 0; i < ops.size(); i++) {
        u -= ops[i].weight;
        if (u <= 0.0) return i;
    }
    return ops.size() - 1;
}

// ---------------------------------------------------------------------------------------------
// Destroy

void LNS::destroy(int op, int q)
{
    locate();
    vector<char> mark(nSites, 0);
    switch (op) {
        case 0: randomRemoval(q, mark); break;
        case 1: worstRemoval(q, mark); break;
        case 2: relatedRemoval(q, mark, {}); break;
        case 3: routeRemoval(mark); break;
        default: lateRemoval(q, mark); break;
    }
    removeMarked(mark);
}

void LNS::removeMarked(const vector<char>& mark)
{
    removed.clear();
    for (int c = 1; c < nSites; c++)
        if (mark[c]) removed.push_back(c);
    for (int r = 0; r < (int)inst->getRoutes().size(); r++) {
        vector<int>& s = seq(r);
        if (std::none_of(s.begin(), s.end(), [&](int c) { return mark[c]; })) continue;
        touch(r);
        s.erase(std::remove_if(s.begin(), s.end(), [&](int c) { return mark[c]; }), s.end());
        buildSchedule(r);
    }
}

void LNS::randomRemoval(int q, vector<char>& mark)
{
    vector<int> customers(nCustomers);
    std::iota(customers.begin(), customers.end(), 1);
    std::shuffle(customers.begin(), customers.end(), rng);
    for (int i = 0; i < q && i < nCustomers; i++) mark[customers[i]] = 1;
}

// the stores whose detour (and lateness) cost most, with some randomness
void LNS::worstRemoval(int q, vector<char>& mark)
{
    vector<pair<double, int>> saving;
    for (int r = 0; r < (int)inst->getRoutes().size(); r++) {
        const vector<int>& s = seq(r);
        const vector<int>& lateness = inst->getRoutes()[r].getLatenessPerStop();
        for (int p = 0; p < (int)s.size(); p++) {
            int prev = (p == 0) ? 0 : s[p - 1];
            int next = (p + 1 < (int)s.size()) ? s[p + 1] : 0;
            double value = D[prev][s[p]] + D[s[p]][next] - D[prev][next];
            if (p < (int)lateness.size() && lateness[p] > 0) value += w_lateness + w_tardiness * lateness[p];
            saving.push_back({value, s[p]});
        }
    }
    std::sort(saving.begin(), saving.end(), [](auto& a, auto& b) { return a.first > b.first; });
    std::uniform_real_distribution<double> u(0.0, 1.0);
    for (int i = 0; i < q && !saving.empty(); i++) {
        size_t idx = std::min(saving.size() - 1, (size_t)(std::pow(u(rng), 3.0) * saving.size()));
        mark[saving[idx].second] = 1;
        saving.erase(saving.begin() + idx);
    }
}

// Shaw removal: stores close in space, service time and demand to ones already removed
void LNS::relatedRemoval(int q, vector<char>& mark, vector<int> seeds)
{
    std::uniform_real_distribution<double> u(0.0, 1.0);
    vector<int> chosen;
    for (int c : seeds)
        if (!mark[c]) { mark[c] = 1; chosen.push_back(c); }
    if (chosen.empty()) {
        int c = std::uniform_int_distribution<int>(1, nCustomers)(rng);
        mark[c] = 1;
        chosen.push_back(c);
    }
    auto start = [&](int c) { return std::max(sched[routeOf[c]].arrive[posOf[c]], ready[c]); };

    vector<pair<double, int>> related;
    while ((int)chosen.size() < q) {
        int ref = chosen[std::uniform_int_distribution<int>(0, chosen.size() - 1)(rng)];
        related.clear();
        for (int c = 1; c < nSites; c++) {
            if (mark[c]) continue;
            related.push_back({9.0 * D[ref][c] / maxDistance
                             + 3.0 * std::abs(start(ref) - start(c)) / (double)horizon
                             + 2.0 * std::abs(demand[ref] - demand[c]) / maxDemand, c});
        }
        if (related.empty()) break;
        std::sort(related.begin(), related.end());
        size_t idx = std::min(related.size() - 1, (size_t)(std::pow(u(rng), 6.0) * related.size()));
        mark[related[idx].second] = 1;
        chosen.push_back(related[idx].second);
    }
}

// every store of one route, the shortest of three picked at random, so a vehicle can be freed
void LNS::routeRemoval(vector<char>& mark)
{
    vector<int> used;
    for (int r = 0; r < (int)inst->getRoutes().size(); r++)
        if (!seq(r).empty()) used.push_back(r);
    if (used.empty()) return;
    std::uniform_int_distribution<int> any(0, used.size() - 1);
    int r = used[any(rng)];
    for (int t = 0; t < 2; t++) {
        int other = used[any(rng)];
        if (seq(other).size() < seq(r).size()) r = other;
    }
    for (int c : seq(r)) mark[c] = 1;
}

// late stores (and the last stop of a route back late), then stores related to them
void LNS::lateRemoval(int q, vector<char>& mark)
{
    vector<int> late;
    for (int r = 0; r < (int)inst->getRoutes().size(); r++) {
        const vector<int>& s = seq(r);
        const vector<int>& lateness = inst->getRoutes()[r].getLatenessPerStop();
        for (int p = 0; p < (int)s.size() && p < (int)lateness.size(); p++)
            if (lateness[p] > 0) late.push_back(s[p]);
        if (!s.empty() && (int)lateness.size() > (int)s.size() && lateness[s.size()] > 0) late.push_back(s.back());
    }
    std::shuffle(late.begin(), late.end(), rng);
    if ((int)late.size() > q) late.resize(q);
    relatedRemoval(q, mark, late);
}

// ---------------------------------------------------------------------------------------------
// Repair: regret-k insertion (k = 1 is greedy). Each round inserts the store with the largest
// regret (sum over the next k-1 best routes of how much worse they are than its best route) at
// its cheapest position. Only one empty route is considered, since they are all alike.

void LNS::repair(int k, bool noise)
{
    int R = inst->getRoutes().size();
    int Q = removed.size();
    if (Q == 0) return;

    std::uniform_real_distribution<double> jitter(-NOISE * maxDistance, NOISE * maxDistance);
    auto firstEmpty = [&]() {
        for (int r = 0; r < R; r++)
            if (seq(r).empty()) return r;
        return -1;
    };
    int emptyRep = firstEmpty();

    vector<vector<double>> c(Q, vector<double>(R, INF));
    vector<vector<int>> pos(Q, vector<int>(R, -1));
    auto evalRoute = [&](int i, int r) {
        c[i][r] = INF;
        pos[i][r] = -1;
        if (seq(r).empty() && r != emptyRep) return;
        int m = seq(r).size();
        for (int p = 0; p <= m; p++) {
            double v = insertionCost(removed[i], r, p);
            if (noise) v += jitter(rng);
            if (v < c[i][r]) { c[i][r] = v; pos[i][r] = p; }
        }
    };
    for (int i = 0; i < Q; i++)
        for (int r = 0; r < R; r++) evalRoute(i, r);

    vector<char> done(Q, 0);
    vector<double> sorted(R);
    for (int step = 0; step < Q; step++) {
        int chosen = -1;
        double chosenRegret = -INF, chosenCost = INF;
        for (int i = 0; i < Q; i++) {
            if (done[i]) continue;
            sorted = c[i];
            int kk = std::min(k, R);
            std::partial_sort(sorted.begin(), sorted.begin() + kk, sorted.end());
            double regret = 0.0;
            for (int h = 1; h < kk; h++)
                regret += (sorted[h] >= INF / 2) ? 1e15 : sorted[h] - sorted[0];
            if (regret > chosenRegret + 1e-9 || (std::abs(regret - chosenRegret) <= 1e-9 && sorted[0] < chosenCost)) {
                chosen = i;
                chosenRegret = regret;
                chosenCost = sorted[0];
            }
        }

        int r = std::min_element(c[chosen].begin(), c[chosen].end()) - c[chosen].begin();
        touch(r);
        vector<int>& s = seq(r);
        s.insert(s.begin() + pos[chosen][r], removed[chosen]);
        buildSchedule(r);
        done[chosen] = 1;

        bool filledEmpty = (r == emptyRep);
        if (filledEmpty) emptyRep = firstEmpty();
        for (int i = 0; i < Q; i++) {
            if (done[i]) continue;
            evalRoute(i, r);
            if (filledEmpty && emptyRep >= 0) evalRoute(i, emptyRep);
        }
    }
    removed.clear();
}

// ---------------------------------------------------------------------------------------------
// Polish: 2-opt and or-opt (move 1-3 consecutive stops) within a route. A move is tried in the
// full simulation only if the estimate says it improves, and kept only if the simulation agrees.

bool LNS::tryRoute(int r, const vector<int>& candidate)
{
    Route& route = inst->getRoutes()[r];
    Route old = route;
    route.setRoute(candidate);
    RouteCost c = evaluate(r);
    if (c.cost < cost[r].cost - 1e-9) {
        cost[r] = c;
        buildSchedule(r);
        return true;
    }
    route = old;
    return false;
}

void LNS::polishRoute(int r)
{
    for (int pass = 0; pass < 1000; pass++) {
        vector<int> s = seq(r);
        int m = s.size();
        if (m < 2) return;
        double base = estimateCost(s);
        bool improved = false;

        for (int i = 0; i < m - 1 && !improved; i++)
            for (int j = i + 1; j < m && !improved; j++) {
                vector<int> candidate = s;
                std::reverse(candidate.begin() + i, candidate.begin() + j + 1);
                if (estimateCost(candidate) < base - 1e-9 && tryRoute(r, candidate)) improved = true;
            }

        for (int len = 1; len <= 3 && !improved; len++)
            for (int i = 0; i + len <= m && !improved; i++) {
                vector<int> rest(s.begin(), s.begin() + i);
                rest.insert(rest.end(), s.begin() + i + len, s.end());
                for (int p = 0; p <= (int)rest.size() && !improved; p++) {
                    if (p == i) continue;
                    vector<int> candidate = rest;
                    candidate.insert(candidate.begin() + p, s.begin() + i, s.begin() + i + len);
                    if (estimateCost(candidate) < base - 1e-9 && tryRoute(r, candidate)) improved = true;
                }
            }

        if (!improved) return;
    }
}

// ---------------------------------------------------------------------------------------------

void LNS::saveBest()
{
    best.clear();
    for (const Route& route : inst->getRoutes()) best.push_back(route.getRoute());
}

void LNS::restoreBest()
{
    currentCost = 0.0;
    for (int r = 0; r < (int)best.size(); r++) {
        inst->getRoutes()[r].setRoute(best[r]);
        cost[r] = evaluate(r);
        buildSchedule(r);
        currentCost += cost[r].cost;
    }
}

void LNS::writeRoutes(const string& filename, const vector<vector<int>>& routes) const
{
    std::ofstream out(filename);
    int id = 0;
    for (const vector<int>& s : routes) {
        if (s.empty()) continue;
        out << id++ << " :";
        for (int c : s) out << " " << c;
        out << "\n";
    }
}

SolutionScore LNS::runLargeNeighbourSearch(int iterations)
{
    bool recordDetail = Route::recordDetail();
    Route::setRecordDetail(false);   // the search needs no per-minute log

    int R = inst->getRoutes().size();
    cost.assign(R, RouteCost());
    sched.assign(R, Schedule());
    touched.assign(R, 0);
    currentCost = 0.0;
    double startDistance = 0.0;
    for (int r = 0; r < R; r++) {
        cost[r] = evaluate(r);
        buildSchedule(r);
        currentCost += cost[r].cost;
        startDistance += cost[r].distance;
    }
    saveBest();
    bestCost = currentCost;
    double startCost = currentCost;

    double temperature = std::max(1.0, START_WORSE * startDistance / std::log(2.0));
    double cooling = std::pow(END_TEMPERATURE, 1.0 / std::max(1, iterations));

    int qMin = std::max(1, std::min(nCustomers, (int)std::lround(std::min(0.1 * nCustomers, 30.0))));
    int qMax = std::max(qMin, std::min(nCustomers, (int)std::lround(std::min(0.4 * nCustomers, 60.0))));

    string bestDir = videoDir + "/best/";
    string newDir = videoDir + "/new/";
    if (VIDEO) {
        std::filesystem::create_directories(bestDir);
        std::filesystem::create_directories(newDir);
    }

    std::ofstream log(progressLogFile, std::ios::out | std::ios::trunc);
    log << "iteration,best,current,candidate,accepted,destroy,repair\n";

    std::uniform_real_distribution<double> u(0.0, 1.0);
    std::uniform_int_distribution<int> qDist(qMin, qMax);

    for (int it = 0; it < iterations; it++) {
        backups.clear();
        int d = pick(destroyOps);
        int rp = pick(repairOps);

        destroy(d, qDist(rng));
        repair(1 + rp / 2, rp % 2 == 1);

        double candidate = currentCost;
        for (const Backup& b : backups) {
            candidate -= b.cost.cost;
            cost[b.r] = evaluate(b.r);
            candidate += cost[b.r].cost;
        }

        bool newBest = candidate < bestCost - 1e-9;
        bool better = candidate < currentCost - 1e-9;
        bool accepted = better || u(rng) < std::exp(-(candidate - currentCost) / temperature);

        log << it << "," << bestCost << "," << currentCost << "," << candidate << "," << accepted << ","
            << destroyOps[d].name << "," << repairOps[rp].name << "\n";
        if (VIDEO) {
            vector<vector<int>> now;
            for (const Route& route : inst->getRoutes()) now.push_back(route.getRoute());
            writeRoutes(newDir + std::to_string(it) + ".txt", now);
        }

        double score = newBest ? SCORE_NEW_BEST : better ? SCORE_BETTER : accepted ? SCORE_ACCEPTED : 0.0;
        destroyOps[d].score += score;
        destroyOps[d].uses++;
        repairOps[rp].score += score;
        repairOps[rp].uses++;

        if (accepted) {
            currentCost = candidate;
            if (newBest) {
                bestCost = candidate;
                saveBest();
            }
        } else {
            for (Backup& b : backups) {
                inst->getRoutes()[b.r] = b.route;
                cost[b.r] = b.cost;
                sched[b.r] = b.schedule;
            }
        }
        for (const Backup& b : backups) touched[b.r] = 0;

        if (VIDEO) writeRoutes(bestDir + std::to_string(it) + ".txt", best);

        temperature *= cooling;
        if ((it + 1) % SEGMENT == 0) {
            for (vector<Operator>* ops : {&destroyOps, &repairOps})
                for (Operator& o : *ops) {
                    if (o.uses > 0) o.weight = std::max(MIN_WEIGHT, (1.0 - REACTION) * o.weight + REACTION * o.score / o.uses);
                    o.score = 0.0;
                    o.uses = 0;
                }
        }
        if (VERBOSE && (it + 1) % std::max(1, iterations / 10) == 0)
            cout << "iteration " << it + 1 << ": best " << bestCost << ", current " << currentCost
                 << ", temperature " << temperature << "\n";
    }

    restoreBest();
    for (int r = 0; r < R; r++) polishRoute(r);
    double polished = 0.0;
    for (int r = 0; r < R; r++) polished += cost[r].cost;
    if (VIDEO) {
        saveBest();
        writeRoutes(bestDir + std::to_string(iterations) + ".txt", best);
    }

    Route::setRecordDetail(recordDetail);
    inst->calculateRoutes();
    if (!inst->checkAllStoresVisitedOnce()) cerr << "[LNS] the solution does not visit every store once" << endl;

    int distance = inst->getTotalDistance();
    int late = inst->getTotalLate();
    int tardiness = inst->getTotalTardiness();
    int overload = inst->getTotalOverload();
    double total = w_distance * distance + w_lateness * late + w_tardiness * tardiness + w_capacity * overload;
    if (std::abs(total - polished) > 0.5)
        cerr << "[LNS] tracked cost " << polished << " differs from a fresh simulation's " << total << endl;

    std::sort(destroyOps.begin(), destroyOps.end(), [](auto& a, auto& b) { return a.weight > b.weight; });
    std::sort(repairOps.begin(), repairOps.end(), [](auto& a, auto& b) { return a.weight > b.weight; });
    cout << "LNS: cost " << startCost << " -> " << bestCost << " (search) -> " << polished << " (polished); weights";
    for (const Operator& o : destroyOps) cout << " " << o.name << "=" << o.weight;
    for (const Operator& o : repairOps) cout << " " << o.name << "=" << o.weight;
    cout << endl;

    return {w_distance * distance, (int)(w_lateness * late + w_tardiness * tardiness), (int)(w_capacity * overload), total};
}
