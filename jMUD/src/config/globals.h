/******************************************************************************
 * file: globals.h
 *
 * created: 2012-02-19 by jimmy
 * authors: jimmy
 *****************************************************************************/
#ifndef GLOBALS_H
#define GLOBALS_H

#include <limits>
#include "Settings.h"

extern Settings settings;


typedef uint64_t ObjectID;
const ObjectID MaxObjectID = std::numeric_limits<ObjectID>::max();
const ObjectID InvalidObjectID = 0;

//extern int rand(int min, int max);
extern int dice(int size, int num);
extern int d100(void);

#endif
