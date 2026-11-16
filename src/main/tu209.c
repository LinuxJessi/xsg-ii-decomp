#include "common.h"
#include "shared.h"

void *memset(void *destination, int value, unsigned int count);

/* Linear interpolation from start to end by ratio: start + (end - start) * ratio. Used by the keyframe sampler animSampleSegment. */
f32 animLerp(f32 start, f32 end, f32 ratio);

/*
 * Start/end time and start/end value of one keyframe segment, plus the
 * value ceiling animSampleSegment clamps against. Only the fields
 * animSampleSegment reads are modeled; the rest of the owning object is
 * unmodeled here.
 */
typedef struct AnimSegment {
    unsigned char unmodeled_00[0x28];
    int startTime;  /* +0x28 */
    int endTime;    /* +0x2c */
    unsigned char unmodeled_30[0x3c - 0x30];
    int endValue;   /* +0x3c */
    unsigned char unmodeled_40[0x50 - 0x40];
    int startValue; /* +0x50 */
    unsigned char unmodeled_54[0x5c - 0x54];
    int maxValue;   /* +0x5c */
} AnimSegment;

/*
 * Samples the keyframe segment at `position`: interpolates between
 * startValue and endValue by the segment's elapsed fraction and clamps to
 * [0, maxValue]. `out` is a 4-float (0x10 byte) buffer; its first three
 * entries receive the sampled value. Returns 0 when position is before the
 * segment (out zeroed), 2 when past it (out zeroed except out[0] = maxValue),
 * 1 otherwise (out[0..2] = the sampled value).
 */
int animSampleSegment(f32 *out, AnimSegment *seg, f32 position)
{
    int startTime = seg->startTime;
    f32 endValue = (f32)seg->endValue;
    int endTime = seg->endTime;
    f32 ratio = (position - (f32)startTime) / (f32)(endTime - startTime);
    f32 startValue = (f32)seg->startValue;
    int maxValue = seg->maxValue;
    f32 value = animLerp(startValue, endValue, ratio);
    f32 maxValueF;

    if (value < 0.0f) {
        memset(out, 0, 0x10);
        return 0;
    }
    maxValueF = (f32)maxValue;
    if (maxValueF < value) {
        memset(out, 0, 0x10);
        out[0] = maxValueF;
        return 2;
    }
    out[0] = out[1] = out[2] = value;
    return 1;
}

f32 animLerp(f32 start, f32 end, f32 ratio)
{
    return ((end - start) * ratio) + start;
}

/* Offsets x and y of four corners (recordDrawCurve's quad in tu208.c) by (dx, dy); z and w are untouched. */
void animOffsetPoints(Vector4 *points, f32 dx, f32 dy)
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
