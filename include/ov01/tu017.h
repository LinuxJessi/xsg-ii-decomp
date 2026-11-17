#ifndef INCLUDE_OV01_TU017_H
#define INCLUDE_OV01_TU017_H

#include "ov01/tu012.h"

/* Functions of src/ov01/tu017.c that other files call: the table of the unit objects (players, then enemies). */

/* Side argument of unitTblGet and the count helpers: players, enemies, or anything else for both tables. */
enum {
    UNIT_SIDE_PLAYER = 0,
    UNIT_SIDE_ENEMY = 1,
    UNIT_SIDE_ALL = 2
};

int unitTblGet(int side, Ov01Object ***table);
int unitTblSet(int side, Ov01Object *object);
int unitTblRemove(Ov01Object *object);
int unitNumGet(int side);
int unitNumFlag4ClearGet(int side);
int unitNumFlag4ClearStatZeroGet(int side);
int unitNumFlag4SetGet(int side);
int unitNumFlag1Flag4ClearGet(int side);
int unitNumBusyGet(int side);
int unitTblIndexOf(Ov01Object *target);
Ov01Object *unitTblEntryGet(int index);

#endif /* INCLUDE_OV01_TU017_H */
