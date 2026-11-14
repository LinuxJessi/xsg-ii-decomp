#ifndef INCLUDE_MAIN_TIWATSIM_H
#define INCLUDE_MAIN_TIWATSIM_H

#include "shared.h"

/* One water simulation item (0x110 bytes) of a TiWaterSub table (src/main/TiWatSim.c, TiWater.c). */
typedef struct TiWatSimItem {
    u8 unmodeled_00[0x04];
    int cell_count;
    int cols;
    int rows;
    u8 unmodeled_10[0x30];
    float grid_origin_x;
    u8 unmodeled_44[0x04];
    float grid_origin_z;
    u8 unmodeled_4C[0x04];
    float cell_spacing;
    u8 unmodeled_54[0x04];
    float drop_amount;
    u8 unmodeled_5C[0x0C];
    float *height_buffer_a;
    float *height_buffer_b;
    u8 unmodeled_70[0x08];
    u32 mask_offset;
    u32 normals_offset;
    u8 unmodeled_80[0x04];
    u8 *block;
    u8 unmodeled_88[0x48];
    Matrix4 matrix;
} TiWatSimItem;

#endif /* INCLUDE_MAIN_TIWATSIM_H */
