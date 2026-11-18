#ifndef INCLUDE_OV01_TU004_H
#define INCLUDE_OV01_TU004_H

#include "ov01/tu005.h"

/* Functions of src/ov01/tu004.c that other files call, and the data object record they share with tu022. */

struct DataVec3 {
    float x;
    float y;
    float z;
};

/* Pose block of an object at +0x150: position, the position tail and the anchor written by
 * dataObjGetPosition and dataTargetApply (the anchor is a union view, alias set 0). */
struct DataPose {
    struct DataVec3 position;
    unsigned char unmodeled_0C[0x14 - 0x0C];
    int position_tail;
    unsigned char unmodeled_18[0x60 - 0x18];
    union { struct DataVec3 vec; } anchor;
};

/* One of the two 0x1E0-byte data object slots (dataObjTable) and the object made in it by dataObjCreate (tu022, global
 * dataObject), also the target block of the key and cursor code. 0x00 slot flags (OV01_SLOT_* of tu000.h: claim and
 * the update and draw skip bits of the sweeps in tu004), 0x04 update callback (dataObjUpdate), 0x08 draw callback
 * (dataObjDraw), 0x0C object flags (0x2 hidden, 0x1 set by dataTargetMatchKey), 0x10 selected index (the entry
 * dataCursorMove compares), 0x14 lookup block, 0x150 pose, 0x1D0 id, 0x1D4 kind. */
struct DataObject {
    /* 0x00 */ int slotFlags;
    /* 0x04 */ void (*update)(struct DataObject *obj);
    /* 0x08 */ void (*draw)(struct DataObject *obj);
    /* 0x0C */ int flags;
    /* 0x10 */ int selectedIndex;
    /* 0x14 */ struct DataLookup lookup; /* the lookup block (tu005.h); the pad below keeps the pose at 0x150 */
    /* 0x140 */ unsigned char unmodeled_140[0x150 - 0x140];
    /* 0x150 */ struct DataPose pose;
    /* 0x1BC */ unsigned char unmodeled_1BC[0x1D0 - 0x1BC];
    /* 0x1D0 */ void *id;
    /* 0x1D4 */ int kind;
    /* 0x1D8 */ unsigned char unmodeled_1D8[0x1E0 - 0x1D8];
};

struct DataObject *dataObjAlloc(void);
void dataObjDrawAll(void);
void dataObjUpdateAll(void);

#endif /* INCLUDE_OV01_TU004_H */
