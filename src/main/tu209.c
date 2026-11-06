#include "common.h"

INCLUDE_ASM("asm/main/nonmatchings/tu209", animSampleSegment);

#include "shared.h"

/* Linear interpolation from start to end by ratio: start + (end - start) * ratio. Used by the keyframe sampler animSampleSegment. */
f32 animLerp(f32 start, f32 end, f32 ratio)
{
    return ((end - start) * ratio) + start;
}

/*
 * One 0x10-byte entry of the object animOffsetPoints updates.  Only the x and
 * y words are evidenced by this function; the rest of the entry is untouched
 * here and stays unmodeled.
 */
typedef struct AnimPoint {
    f32 x;
    f32 y;
    unsigned char unmodeled_08[8];
} AnimPoint;

/* Offsets the x and y words of the object's four points by (dx, dy). */
void animOffsetPoints(AnimPoint *points, f32 dx, f32 dy)
{
    points[0].y += dy;
    points[0].x += dx;
    points[1].y += dy;
    points[1].x += dx;
    points[2].y += dy;
    points[2].x += dx;
    points[3].y += dy;
    points[3].x += dx;
}
