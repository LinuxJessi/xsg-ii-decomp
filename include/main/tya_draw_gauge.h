#ifndef INCLUDE_MAIN_TYA_DRAW_GAUGE_H
#define INCLUDE_MAIN_TYA_DRAW_GAUGE_H

/* Data of src/main/tya_draw_gauge.c that other files read (needs shared.h for Vector4). */

#define UWAMONO_POINT_COUNT 16

/* Point table entry, read by tu083.c. */
typedef struct UwamonoPoint {
    Vector4 position;                   /* +0x00: copied as one 16-byte block */
    short key;                          /* +0x10 */
    unsigned char unmodeled_12[2];
    float value;                        /* +0x14 */
    unsigned char unmodeled_18[8];
} UwamonoPoint;

extern UwamonoPoint uwamonoPoints[UWAMONO_POINT_COUNT];

#endif /* INCLUDE_MAIN_TYA_DRAW_GAUGE_H */
