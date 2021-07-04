#include "config.h"
#include "WorldEngine.h"
#include "WorldZone.h"
#include "WorldRoom.h"
#include "log.h"

#include <fstream>
#include <dirent.h>     // DIR, opendir(), readdir(), closedir()
#include <QString>



const char *Exits[] = { "North", "East", "Up", "South", "West", "Down" };
const char *STR_Directions[] = { "north", "east", "up", "south", "west", "down" };
const char *STR_ArrivalDirections[] = { "the south", "the west", "below", "the north", "the east", "above" };
const char *OppositeExits[] = { "South", "West", "Down", "North", "East", "Up" };



bool WorldEngine::initialize(void) {
    log_INIT();

    if (LoadWorld() == false) {
        return false;
    }

    log_INIT_OK();
    return true;
}


bool WorldEngine::LoadWorld(void) {
    const char *path = settings.getSetting("server.game.world");
    if (path == NULL) {
        path = "./data/world";
    }
    sys::log::WorldEngine::add("world location = %s", path);

    DIR* directory = opendir(path);
    if (directory == NULL) {
        sys::log::WorldEngine::error("Unable to open/read world directory: '%s' (%i:%s)", path, errno, strerror(errno));
        return false;
    }

    struct dirent* file = NULL;
    std::size_t length = 0;
    char zonefile[1024];

    while ((file = readdir(directory)) != NULL) {
        // 6 chars is the minimum for a valid zone file name.
        if ((length = strlen(file->d_name)) < 6)
            continue;

        // Does the filename end with ".zone"? If so load a zone from it.
        if (strcmp((file->d_name+length-5), ".zone") == 0) {
            strcpy(zonefile, path);
            strcat(zonefile, file->d_name);
            LoadZone(zonefile);
        }
    };
    closedir(directory);
    sys::log::WorldEngine::add("Loaded: %i room(s) in %i zone(s)", rooms.size(), zones.size());


    // Here the rooms should be linked to each other by their pointers.
    WorldRoom* tmpRoom, *room;
    std::list<WorldRoom*>::iterator iroom = rooms.begin();
    while (iroom != rooms.end()) {
        room = (*iroom);
        for (int i = 0; i < NUM_DIRECTIONS; i++) {
            if ((room->directionsVNum[i] == INVALID_VNUM) || (room->directions[i] != NULL))
                continue;

            tmpRoom = FindRoom(room->directionsVNum[i]);

            if (tmpRoom == NULL) {
                sys::log::WorldEngine::error("Reference to non-existing room %i", room->directionsVNum[i]);
            } else {
                room->directions[i] = tmpRoom;
                if ((room->directionsVNum[(i+(NUM_DIRECTIONS/2))%NUM_DIRECTIONS]) == room->rnum) {
                    tmpRoom->directions[(i+(NUM_DIRECTIONS/2))%NUM_DIRECTIONS] = room;
                }
            }
        }
        ++iroom;
    }

//    PrintWorld();

    room_start = FindRoom(12);
    if (room_start == NULL) {
        sys::log::WorldEngine::error("Failed to find starting room.");
        return false;
    }
    return true;
}


bool WorldEngine::LoadZone(const char* filename) {
    assert(filename != NULL);

    std::fstream file(filename, std::ios::in);
    if (file.is_open() == 0) {
        sys::log::WorldEngine::error("LoadZone: Could not open file: %s (%i:%s)", filename, errno, strerror(errno));
        return false;
    }
    WorldZone* zone = new WorldZone();

    char lineBuffer[1024], str[1024];
    int vnum = 0, value = 0;

    sys::log::WorldEngine::debug("  zone: %s - loading", filename);
    while (!file.eof()) {
        file >> std::ws;
        file.get(lineBuffer, 1023, '\n');

        // Ignore comment lines.
        if (lineBuffer[0] == '/')
            continue;

        if (sscanf(lineBuffer, "# [%i]", &vnum) == 1) {
            WorldRoom* room = LoadRoom(file, vnum);
            if (room != NULL) {
                zone->AddRoom(room);
                this->AddRoom(room);
            }
            continue;
        }

        if (sscanf(lineBuffer, "version=%i", &value) == 1) {
            // TODO: Handle version value.
            continue;
        }

        if (sscanf(lineBuffer, "zone=%[^\n]s", str) == 1) {
            zone->name = str;
            continue;
        }

        if (sscanf(lineBuffer, "zone_vnum=%i", &value) == 1) {
            // TODO: Handle version value.
            zone->znum = value;
            continue;
        }

        if (sscanf(lineBuffer, "zone_author=%[^\n]s", str) == 1) {
            // TODO: Handle
            continue;
        }
        if (sscanf(lineBuffer, "date_created=%[^\n]s", str) == 1) {
            // TODO: Handle
            continue;
        }
        if (sscanf(lineBuffer, "zone_owner=%[^\n]s", str) == 1) {
            // TODO: Handle
            continue;
        }
        if (sscanf(lineBuffer, "date_updated=%[^\n]s", str) == 1) {
            // TODO: Handle
            continue;
        }

        if (sscanf(lineBuffer, "zone_desc=%[^\n]s", str) == 1) {
            zone->description = str;
            continue;
        }

        if (sscanf(lineBuffer, "zone_notes=%[^\n]s", str) == 1) {
            // TODO: Handle
            continue;
        }
        if (sscanf(lineBuffer, "zone_spawn_room=%i", &value) == 1) {
            // TODO: Handle
            continue;
        }


        sys::log::WorldEngine::debug("    unknown line: '%s'", vnum, lineBuffer);
        file >> std::ws;
    }
    if (zone->name.isEmpty() == false && zone->description.isEmpty() == false && zone->znum != INVALID_VNUM) {
        sys::log::WorldEngine::add("loaded zone: %s (%i rooms)", qPrintable(zone->name), zone->rooms.size());
        AddZone(zone);
        return true;
    }
    sys::log::WorldEngine::debug("  zone: %s - loading - FAILED", filename);
    return false;
}


/***
 *
 */
WorldRoom* WorldEngine::LoadRoom(std::fstream &file, int vnum) {

    sys::log::WorldEngine::debug("    room %i: loading", vnum);
    char lineBuffer[1024], str[1024];
    int n = 0, e = 0, u = 0, s = 0, w = 0, d = 0;
    bool foundDirections = false;
    WorldRoom* room = new WorldRoom();
    room->rnum = vnum;

    while (!file.eof()) {
        file >> std::ws;
        if (file.peek() == '#')
            break;
        file.get(lineBuffer, 1023, '\n');

        // Ignore comment lines.
        if (lineBuffer[0] == '/')
            continue;

        if (sscanf(lineBuffer, "room=%[^\n]s", str) == 1) {
            room->name = str;
            continue;
        }

        if (sscanf(lineBuffer, "room_desc=%[^\n]s", str) == 1) {
            room->description = str;
            continue;
        }

        if (sscanf(lineBuffer, "room_flags=%[^\n]s", str) == 1) {
            // TODO: Add flags and handling of them.
            continue;
        }

        n = e = u = s = w = d = 0;
        if (sscanf(lineBuffer, "exits=north :%i east :%i up :%i south :%i west :%i down :%i", &n, &e, &u, &s, &w, &d) == 6) {
            room->directionsVNum[DIRECTION_NORTH] = n;
            room->directionsVNum[DIRECTION_EAST] = e;
            room->directionsVNum[DIRECTION_UP] = u;
            room->directionsVNum[DIRECTION_SOUTH] = s;
            room->directionsVNum[DIRECTION_WEST] = w;
            room->directionsVNum[DIRECTION_DOWN] = d;
            foundDirections = true;
            continue;
        }

        sys::log::WorldEngine::debug("      unknown line: '%s'", lineBuffer);
        file >> std::ws;
    }

    if (room->name.isEmpty() == false && room->description.isEmpty() == false && foundDirections == true) {
        sys::log::WorldEngine::debug("    room %i: loading - done", vnum);
        return room;
    }

    delete room;
    sys::log::WorldEngine::debug("    room %i: loading - FAILED", vnum);
    return NULL;
}


/***
 * Searches after a specific roomnumber throughout all the world.
 *
 * NOTE: For now it's a linear search from the start to the end of an unordered
 *       linked list which will make it quite inefficient when the worldsize
 *       grows, though it's quite simple to alter the datastructure used to
 *       store the rooms, and also all methods that manipulate/access this.
 */
WorldRoom* WorldEngine::FindRoom(int vnum) {
    std::list<WorldRoom*>::iterator iroom = rooms.begin();
    while (iroom != rooms.end()) {
        if ((*iroom)->rnum == vnum)
            return (*iroom);
        ++iroom;
    }
    return NULL;
}


void WorldEngine::PrintWorld(void) {

    printf("*******************************************************************************\n");
    std::list<WorldRoom*>::iterator iroom = rooms.begin();
    while (iroom != rooms.end()) {

        printf("room: %i\n", (*iroom)->rnum);
        printf("  name: %s\n", qPrintable((*iroom)->name));
        printf("  desc: %s\n", qPrintable((*iroom)->description));
        printf("  exit: ");
        for (int i = 0; i < NUM_DIRECTIONS; i++) {
            if ((*iroom)->directionsVNum[i] != INVALID_VNUM)
                printf("%s=%i ", STR_Directions[i], (*iroom)->directionsVNum[i]);
        }
        printf("\n");
        ++iroom;
    }
    printf("*******************************************************************************\n");
}
