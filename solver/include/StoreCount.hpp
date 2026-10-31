#ifndef STORECOUNT_H
#define STORECOUNT_H

using namespace std;

struct StoreCount {
    int store_id;
    int count;
    std::vector<int> routes;  // List of route indices
};

#endif