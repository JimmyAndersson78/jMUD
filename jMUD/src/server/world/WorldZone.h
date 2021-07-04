#ifndef WORLDZONE_H
#define WORLDZONE_H

#include "world.h"
#include <list>
#include <cassert>
#include <QString>



class WorldZone {
    friend class WorldEngine;

public:
    WorldZone(void);
    ~WorldZone(void);

private:
    void AddRoom(WorldRoom *room);

    int znum;
    QString name;     // Title for the zone.
    QString description;
    time_t timeCreation;

    std::list<WorldRoom*> rooms;
};


inline WorldZone::WorldZone(void) : znum(0), name(), description(), timeCreation(0), rooms() {
}


inline WorldZone::~WorldZone(void) {
    rooms.clear();
}


inline void WorldZone::AddRoom(WorldRoom *room) {
    assert(room != NULL);
    rooms.push_back(room);
}

#endif // WORLDZONE_H
