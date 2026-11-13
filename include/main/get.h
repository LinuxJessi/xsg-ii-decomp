#ifndef INCLUDE_MAIN_GET_H
#define INCLUDE_MAIN_GET_H

#include "shared.h"

/* The request block of the map probes (mapProbeInit, mapMoveProbe) that the Undu helpers in get.c fill. */
typedef struct UnduParam {
    int query_flags;
    unsigned char unmodeled_04[0x4];
    short attr_mask;
    unsigned char unmodeled_0A[5];
    unsigned char probeEnabled;         /* +0x0F: copied from the actor's probeEnabled */
    unsigned char unmodeled_10[8];
    union {
        int header;                     /* +0x18: the map block entry mapFindBlockEntry returns */
        struct LayoutHeader *layout;    /* +0x18: the same word as the layout header (X1 UnduParam.header) */
    };
    unsigned char unmodeled_1C[0x4];
    int attribute;
    unsigned char unmodeled_24[0x4];
    float scale;                        /* +0x28: copied from uwamonoScaleRecords[slot].scale */
} UnduParam;

/* Functions of src/main/get.c that other files call. */

int Get_Rnd(int min, int max);
void Get_MiddlePoint(const Vector4 *start, const Vector4 *following,
                     short point_index, short point_count, Vector4 *result);
void Get_MiddlePoint_Curve(const Vector4 *start, const Vector4 *following, short point_index, short point_count,
                           const float *curve_weights, const float *curve_values, short sample_count, Vector4 *result);
float unduGetHeightAttr(const Vector4 *position, int unused, int attr_mask, UnduParam *request);
int unduGetAttr(const Vector4 *position, int unused, int attr_mask);
int unduGetMapAttr(const Vector4 *position, int map_key, int attr_mask);

#endif /* INCLUDE_MAIN_GET_H */
