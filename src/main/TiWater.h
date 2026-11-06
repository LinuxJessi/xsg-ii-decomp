#ifndef MAIN_TIWATER_H
#define MAIN_TIWATER_H

/* Object whose word at +0xC0 is the offset of a sub-object from its base. */
struct TiWaterObject {
    unsigned char unmodeled_00[0xC0];
    int sub_offset;
};

/* Sub-object reached through TiWaterObject.sub_offset; its word at +0x08 gates the release path. */
struct TiWaterSub {
    unsigned char unmodeled_00[0x8];
    int active;
};

#endif
