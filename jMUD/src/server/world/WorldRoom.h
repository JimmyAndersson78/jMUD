#ifndef WORLDROOM_H
#define WORLDROOM_H

#include "world.h"
#include <QString>



class WorldRoom {
    friend class WorldZone;
    friend class WorldEngine;

public:
    WorldRoom(void);
    ~WorldRoom(void);

private:
    WorldRoom(const WorldRoom&);
    WorldRoom& operator=(const WorldRoom&);

    int rnum;               // Virtual number for this room.
    QString name;           // Room title/name to identify the room
    QString description;    // Brief description for this room

    WorldRoom *directions[NUM_DIRECTIONS];  // Room objects in all directions.
    int directionsVNum[NUM_DIRECTIONS];     // Room vnum in all directions.
};


inline WorldRoom::WorldRoom() : rnum(0), name(), description(), directions(), directionsVNum() {
    for (int i = 0; i < NUM_DIRECTIONS; i++) {
        directions[i] = NULL;
        directionsVNum[i] = INVALID_VNUM;
    }
}


inline WorldRoom::~WorldRoom() {
}

#endif // WORLDROOM_H
