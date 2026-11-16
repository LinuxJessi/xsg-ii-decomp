#ifndef MAIN_TIWATER_H
#define MAIN_TIWATER_H

#include "main/TiWatSim.h"

/* Object whose word at +0xC0 is the offset of a sub-object from its base. */
struct TiWaterObject {
    unsigned char unmodeled_00[0xC0];
    int sub_offset;
};

/* The sub-object reached through TiWaterObject.sub_offset is a TiWaterSub (main/TiWatSim.h). */

#endif
