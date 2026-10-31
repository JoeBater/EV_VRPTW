#ifndef STOREDATA_H
#define STOREDATA_H

using namespace std;

struct StoreData {
        int id;
        int demand;
        int ready_time;
        int due_time;
        int service_time;
        int x;
        int y;
        double elevation;
    };

#endif