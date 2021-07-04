#ifndef WORLDENGINE_H
#define WORLDENGINE_H

#include "config.h"
#include "world.h"
#include "WorldZone.h"
#include "WorldRoom.h"
#include <list>



class WorldEngine {

public:
    static WorldEngine &instance(void);
    bool initialize(void);
    int  shutdown(void);
    int  update(void);

    WorldRoom* GetStartRoom(void);
    WorldRoom* FindRoom(int);

private:
    WorldEngine(void);
    WorldEngine(const WorldEngine&);
    ~WorldEngine(void);
    WorldEngine& operator=(const WorldEngine&);

    void       AddZone(WorldZone* zone);
    void       AddRoom(WorldRoom* room);

    bool       LoadWorld(void);
    bool       LoadZone(const char* filename);
    WorldRoom* LoadRoom(std::fstream &file, int vnum);

    std::list<WorldZone*> zones;
    std::list<WorldRoom*> rooms;

    WorldRoom *room_start;

    void PrintWorld(void);
};


inline WorldEngine::WorldEngine(void) : zones(), rooms(), room_start(NULL)
{
}


inline WorldEngine::~WorldEngine(void) {
    std::list<WorldZone*>::iterator izones = zones.begin();
    while (izones != zones.end()) {
        delete (*izones);
        ++izones;
    }
    zones.clear();

    std::list<WorldRoom*>::iterator irooms = rooms.begin();
    while (irooms != rooms.end()) {
        delete (*irooms);
        ++irooms;
    }
    rooms.clear();
}


inline WorldEngine& WorldEngine::instance () {
    static WorldEngine instanceOfWorldEngine;
    return instanceOfWorldEngine;
}


inline WorldRoom* WorldEngine::GetStartRoom(void) {
    return room_start;
}


inline void WorldEngine::AddZone(WorldZone* zone) {
    assert(zone != NULL);
    zones.push_back(zone);
}


inline void WorldEngine::AddRoom(WorldRoom* room) {
    assert(room != NULL);
    rooms.push_back(room);
}


#endif // WORLDENGINE_H


/*


// Check if we can locate and access the WorldIndex file. If we can't do
// that correctly see what's the problem and decide what to do after that.
if ((access(filename, 06)) != -1) {

    log_ADD("  Loading world index from: '%s'", filename);
    lFile.open(filename, std::ios::in);
    readSuccess = LoadIndex(lFile);
    lFile.close();

    if (readSuccess == true) return true;
    log_ERROR("WorldEngine: The world index could not be read correctly!");
    return false;
}

// Does the file exist? Either case is as bad, since either it don't exist
// and we can't do anything or it exists but is inaccessible to us.
if( (access(filename, 00)) == -1) {
    xlog.add("No WorldIndex file found, can't load world.");
} else {
    xlog.add("SYSERR: The WorldIndex file is inaccessible.");
}
return false;







/ Lines starting with a backslash '/' are comment lines and will be
/ ignored, which will cause a problem if we want to write zone files
/ and keep comments.
version=1
zone=The Void
zone_vnum=1
zone_author=Daemos
date_created=2012-02-19 13:58:01.938291 +01:00
zone_owner=Daemos
date_updated= 2012-02-19 13:58:01.938291 +01:00

zone_desc=A simple zone intended for testing basic game functionality.
zone_notes=1) Don't use this zone in any live game.
zone_spawn_room=12

# [1]
room=The Town Square
room_desc=A large square with a lot of people milling around, and a well in the middle. Among the merchants you notice a fair amount of adventurers, like yourself.
room_flags=
exits=north :3 east :4 up :0 south :5 west :2 down :0


*/
