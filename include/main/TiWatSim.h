#ifndef INCLUDE_MAIN_TIWATSIM_H
#define INCLUDE_MAIN_TIWATSIM_H

#include "shared.h"

/* One water simulation item (0x110 bytes) of a TiWaterSub table (src/main/TiWatSim.c, TiWater.c). */
typedef struct TiWatSimItem {
    u32 flags;                  /* +0x00: 0x2 and 0x4 select the two draw passes (tiWaterDrawItem), 0x100 set up */
    int cell_count;
    int cols;
    int rows;
    Vector4 translate;          /* +0x10: the item transform is translate, rotate Y, scale */
    u8 unmodeled_20[0x04];
    float rotateY;              /* +0x24 */
    u8 unmodeled_28[0x08];
    Vector4 scale;              /* +0x30 */
    float grid_origin_x;
    u8 unmodeled_44[0x04];
    float grid_origin_z;
    u8 unmodeled_4C[0x04];
    float cell_spacing;
    float wave_speed;           /* +0x54 (tiWatSimAdvance) */
    float drop_amount;
    u8 unmodeled_5C[0x04];
    float drawPair[2];          /* +0x60: sent by tiWaterDrawItem (tiWaterSendItemPair) */
    float *height_buffer_a;
    float *height_buffer_b;
    u32 height_offset_a;        /* +0x70: height_buffer_a as an offset from the table (tiWaterRegisterSub) */
    u32 height_offset_b;        /* +0x74 */
    u32 mask_offset;
    u32 normals_offset;
    u8 unmodeled_80[0x04];
    u8 *block;
    u8 unmodeled_88[0x08];
    Matrix4 transform;          /* +0x90: translate, rotate Y, scale (tiWaterRegisterSub) */
    Matrix4 matrix;             /* +0xD0: the inverse of transform */
} TiWatSimItem;

/* A water table as TiWatSim.c reads it: the same shape as TiWaterSub in TiWater.h (magic "WAT " at +0x00,
 * count at +0x08, the items from +0x10). */
typedef struct TiWatSimTable {
    u8 unmodeled_00[0x08];
    u32 count;
    u8 unmodeled_0C[0x04];
} TiWatSimTable;

void tiWatSimStepRain(TiWatSimItem *item);
void tiWatSimUpdate(float step, TiWatSimItem *item);
void tiWatSimDisturbAll(TiWatSimTable *table, Vector4 *pPos);
void tiWatSimStoreSourcePos(const Vector4 *position);

#endif /* INCLUDE_MAIN_TIWATSIM_H */
