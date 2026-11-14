#ifndef MAIN_TIWATER_H
#define MAIN_TIWATER_H

#include "main/TiWatSim.h"

/* Object whose word at +0xC0 is the offset of a sub-object from its base. */
struct TiWaterObject {
    unsigned char unmodeled_00[0xC0];
    int sub_offset;
};

/* Sub-object reached through TiWaterObject.sub_offset; its word at +0x08 gates the release path. */
struct TiWaterSub {
    int magic;                  /* +0x00: "WAT " */
    unsigned char unmodeled_04[4];
    unsigned int active;        /* +0x08: number of items */
    unsigned char unmodeled_0c[4];
    TiWatSimItem items[1];      /* +0x10: `active` items */
};

#endif
