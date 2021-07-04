#ifndef WORLD_H
#define WORLD_H

class WorldEngine;
class WorldZone;
class WorldRoom;


/* ------------------------------------------------------------------------- *
 * WARNING! Do NOT change this unless you know what you are doing, if you do
 *          it's quite safe to add more directions, or renumber the existing.
 * ------------------------------------------------------------------------- */
const int DIRECTION_NORTH       = 0;
const int DIRECTION_EAST        = 1;
const int DIRECTION_UP          = 2;
const int DIRECTION_SOUTH       = 3;
const int DIRECTION_WEST        = 4;
const int DIRECTION_DOWN        = 5;


const int NUM_DIRECTIONS        = 6;
const int INVALID_DIR           = -1;


extern const char *Exits[];
extern const char *OppositeExits[];
extern const char *STR_Directions[];
extern const char *STR_ArrivalDirections[];

const int INVALID_VNUM          = 0;
const int VNum_Invalid          = 0;

#endif // WORLD_H

