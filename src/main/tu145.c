#include "common.h"
#include "shared.h"
#include "main/tu144.h"

/* Normalizes the 2-D vector (*x, *y) in 8.8 fixed point (a zero length counts as 1e-6). The
 * conversion back takes the whole part of the unscaled component times 1 / length, as the original
 * does. */
void cfeNormalizeFixed2(int *x, int *y)
{
    float fx = CFE_FIXED_TO_FLOAT(*x);
    float fy = CFE_FIXED_TO_FLOAT(*y);
    float length = __builtin_sqrtf(fx * fx + fy * fy);

    if (length == 0.0f) {
        length = 1e-06f;
    }
    length = 1.0f / length;
    *x = (int) ((int) fx * length * 256.0f + (int) ((fx * length - (int) fx * length) * 256.0f));
    *y = (int) ((int) fy * length * 256.0f + (int) ((fy * length - (int) fy * length) * 256.0f));
}

/*
 * A key of a time-keyed track. startTime (+4) is the key time; the tracks are
 * singly linked through next (+0x20) in increasing time. Each direction pair
 * is a 2-D unit vector in 8.8 fixed point, normalized by cfeEntryNormalizeDirs
 * and reset to (1, 0) by cfeEntryResetDirs. The word at +8 is an 8.8 angle
 * that cfeChainUnwrapAngles makes continuous along the chain.
 */
typedef struct CfeTrackNode {
    int flags;
    int startTime;
    int angle;                      /* +0x08: 8.8 fixed point */
    int firstDirection[2];
    int secondDirection[2];
    struct CfeTrackNode *prev;      /* +0x1C */
    struct CfeTrackNode *next;
} CfeTrackNode;

void func_0024FC90(CfeTrackNode *track);
CfeTrackNode *cfeTrackFindAtOrBefore(CfeTrackNode *node, int time);

/* Normalizes both direction pairs of the key (cfeNormalizeFixed2 on each). */
void cfeEntryNormalizeDirs(CfeTrackNode *entry)
{
    cfeNormalizeFixed2(&entry->firstDirection[0], &entry->firstDirection[1]);
    cfeNormalizeFixed2(&entry->secondDirection[0], &entry->secondDirection[1]);
}

/* Resets a direction pair to (1, 0) when its flag bit is set: 0x8 for the first pair, 0x10 for the second. */
void cfeEntryResetDirs(CfeTrackNode *entry)
{
    if (entry != 0) {
        int flags = entry->flags;

        if (flags & 8) {
            entry->firstDirection[1] = 0;
            entry->firstDirection[0] = 0x100;
            return;
        }
        if (flags & 0x10) {
            entry->secondDirection[1] = 0;
            entry->secondDirection[0] = 0x100;
        }
    }
}

INCLUDE_ASM("asm/main/nonmatchings/tu145", func_0024FC90);

#define CFE_PI 3.1415927f
#define CFE_TWO_PI 6.2831855f

/* Makes the angles along the track continuous: each key's angle is moved by whole turns until it is within
 * pi of the angle of the key before it. */
void cfeChainUnwrapAngles(CfeTrackNode *node)
{
    CfeTrackNode *next;
    CfeTrackNode *after;
    float previous;
    float current;
    int whole;

    if (node == 0) {
        return;
    }
    next = node->next;
    if (next == 0) {
        return;
    }
    do {
        current = CFE_FIXED_TO_FLOAT(next->angle);
        previous = CFE_FIXED_TO_FLOAT(node->angle);
        if (previous < current) {
            while (__builtin_fabsf(current - previous) > CFE_PI) {
                current -= CFE_TWO_PI;
            }
        } else {
            while (__builtin_fabsf(current - previous) > CFE_PI) {
                current += CFE_TWO_PI;
            }
        }
        whole = (int) current;
        next->angle = (whole << 8) + (int) ((current - (float) whole) * 256.0f);
        node = next;
        after = next->next;
    } while ((next = after) != 0);
}

/* The angle at `time` between the keys `from` and `to`, interpolated linearly. */
float cfeTrackLerp(CfeTrackNode *from, CfeTrackNode *to, float time)
{
    float t = (time - (float) from->startTime) / (float) (to->startTime - from->startTime);
    float end = CFE_FIXED_TO_FLOAT(to->angle);
    float start = CFE_FIXED_TO_FLOAT(from->angle);

    return (end - start) * t + start;
}

extern f32 cfeTimeScale;

/* Cubic in the time elapsed since the key (scaled by cfeTimeScale): coefficients[0..3], Horner form. The time
 * comes first: EABI passes it in $f12 either way, and this order gives the argument setup of its caller
 * func_00250478. */
f32 cfeEntryEvalCubic(f32 time, CfeTrackNode *entry, f32 *coefficients)
{
    f32 elapsed = (time - (f32) entry->startTime) * cfeTimeScale;

    return (elapsed * ((elapsed * ((elapsed * coefficients[0]) + coefficients[1])) + coefficients[2])) + coefficients[3];
}

/* Fits the cubic through the Bezier points (x[i], y[i]) into coefficients[0..3] (tu259.c). */
void func_00295260(f32 *coefficients, const f32 *x, const f32 *y);
extern f32 D_006317B8;

/* The angle at `time` on the Bezier curve between the keys `from` and `to`: from leaves along its second
 * direction, to arrives along its first one (both normalized; a zero length counts as 1e-7). */
float func_00250478(CfeTrackNode *from, CfeTrackNode *to, float time)
{
    f32 x[4];
    f32 y[4];
    f32 coefficients[4];
    f32 arrive[2];
    f32 leave[2];
    float length;

    leave[0] = CFE_FIXED_TO_FLOAT(from->secondDirection[0]);
    leave[1] = CFE_FIXED_TO_FLOAT(from->secondDirection[1]);
    length = __builtin_sqrtf(leave[0] * leave[0] + leave[1] * leave[1]);
    if (length == 0.0f) {
        length = 1e-07f;
    }
    length = 1.0f / length;
    leave[0] *= length;
    leave[1] *= length;
    arrive[0] = CFE_FIXED_TO_FLOAT(to->firstDirection[0]);
    arrive[1] = CFE_FIXED_TO_FLOAT(to->firstDirection[1]);
    length = __builtin_sqrtf(arrive[0] * arrive[0] + arrive[1] * arrive[1]);
    if (length == 0.0f) {
        length = 1e-07f;
    }
    length = 1.0f / length;
    arrive[0] *= length;
    arrive[1] *= length;
    y[0] = CFE_FIXED_TO_FLOAT(from->angle);
    x[0] = from->startTime * D_006317B8;
    x[1] = x[0] + leave[0];
    y[1] = y[0] + leave[1];
    x[3] = to->startTime * D_006317B8;
    x[2] = x[3] - arrive[0];
    y[3] = CFE_FIXED_TO_FLOAT(to->angle);
    y[2] = y[3] - arrive[1];
    func_00295260(coefficients, x, y);
    return cfeEntryEvalCubic(time, from, coefficients);
}

INCLUDE_ASM("asm/main/nonmatchings/tu145", cfeTrackSample);

INCLUDE_ASM("asm/main/nonmatchings/tu145", func_002508C8);

/* 1 when a key of the track has startTime `time`. */
int cfeTrackHasTime(CfeTrackNode *node, int time)
{
    for (; node != 0; node = node->next) {
        if (node->startTime == time) {
            return 1;
        }
    }
    return 0;
}

/* 1 when any of the `count` tracks has a key at `time`. */
int cfeTrackAnyHasTime(CfeTrackNode **tracks, int count, int time)
{
    int i;

    for (i = 0; i < count; i++) {
        if (cfeTrackHasTime(tracks[i], time)) {
            return 1;
        }
    }
    return 0;
}

INCLUDE_ASM("asm/main/nonmatchings/tu145", cfeTrackFindTime);

INCLUDE_ASM("asm/main/nonmatchings/tu145", cfeTrackFindAtOrBefore);

INCLUDE_ASM("asm/main/nonmatchings/tu145", func_00250BB8);

CfeTrackNode *func_00250BB8(CfeTrackNode *node, int time, int direction);

/* The nearest key after `time` (direction > 0) or before it (otherwise) over `count` tracks, found
 * per track with func_00250BB8; null when no track has one. */
CfeTrackNode *func_00250C68(CfeTrackNode **tracks, int count, int time, int direction)
{
    CfeTrackNode *best = 0;
    CfeTrackNode *node;
    int i;

    for (i = 0; i < count; i++) {
        node = func_00250BB8(tracks[i], time, direction);
        if (node == 0) {
            continue;
        }
        if (best == 0) {
            if (direction > 0) {
                if (time < node->startTime) {
                    best = node;
                }
            } else {
                if (node->startTime < time) {
                    best = node;
                }
            }
        } else if (direction > 0) {
            if (time < node->startTime && node->startTime < best->startTime) {
                best = node;
            }
        } else {
            if (node->startTime < time && best->startTime < node->startTime) {
                best = node;
            }
        }
    }
    return best;
}

/* Number of keys from entry to the end of its track. */
int cfeTrackCount(CfeTrackNode *entry)
{
    CfeTrackNode *cursor = entry;
    int count = 0;

    if (cursor != 0) {
        do {
            cursor = cursor->next;
            count += 1;
        } while (cursor != 0);
    }
    return count;
}

/* Number of keys of the track with lower <= startTime <= upper. */
int func_00250D88(CfeTrackNode *node, int lower, int upper)
{
    int count = 0;

    for (; node != 0; node = node->next) {
        if (node->startTime >= lower && node->startTime <= upper) {
            count++;
        }
    }
    return count;
}

/* Number of runs of equal angles along the track (1 for a track whose keys all share it, 0 for none). */
int cfeTrackCountValueRuns(CfeTrackNode *node)
{
    int count = 0;
    int value;

    if (node != 0) {
        value = node->angle;
        count = 1;
        for (; node != 0; node = node->next) {
            if (node->angle != value) {
                count++;
                value = node->angle;
            }
        }
    }
    return count;
}

/* Number of runs of equal angles among the keys with lower <= startTime <= upper (0 for none). */
int func_00250E10(CfeTrackNode *node, int lower, int upper)
{
    int count;
    int value;

    if (node == 0) {
        return 0;
    }
    while (node->startTime < lower) {
        node = node->next;
        if (node == 0) {
            break;
        }
    }
    if (node == 0) {
        return 0;
    }
    value = node->angle;
    count = 1;
    while (node != 0) {
        if (node->angle != value) {
            count++;
            value = node->angle;
        }
        node = node->next;
        if (node == 0 || node->startTime > upper) {
            break;
        }
    }
    return count;
}

/* Moves every key from the first one at or after `time` by `delta`, then updates the track. */
void cfeTrackShiftTimes(CfeTrackNode *track, int time, int delta)
{
    CfeTrackNode *node = cfeTrackFindAtOrBefore(track, time);
    CfeTrackNode *next;

    while (node != 0) {
        next = node->next;
        node->startTime += delta;
        node = next;
    }
    func_0024FC90(track);
}

/* Moves every key from the first one at or after `time` back by `delta`, at most to one tick after the
 * key before it, then updates the track. A zero delta changes nothing. */
void func_00250EF0(CfeTrackNode *track, int time, int delta)
{
    CfeTrackNode *node = cfeTrackFindAtOrBefore(track, time);
    CfeTrackNode *next;
    int *start;
    int gap;

    if (node != 0) {
        if (delta == 0) {
            return;
        }
        if (node->prev != 0) {
            /* Read through a pointer: the original loads startTime again here. */
            start = &node->startTime;
            gap = *start - node->prev->startTime - 1;
            if (gap < delta) {
                delta = gap;
            }
        }
        while (node != 0) {
            next = node->next;
            node->startTime -= delta;
            node = next;
        }
    }
    func_0024FC90(track);
}

/* 1 when every key of the track has the same angle; 0 for an empty track. */
int cfeTrackAllValuesEqual(CfeTrackNode *node)
{
    int *first;
    int value;

    if (node == 0) {
        return 0;
    }
    /* Read through a pointer: the loop's first compare loads the angle again, as in the original. */
    first = &node->angle;
    value = *first;
    for (; node != 0; node = node->next) {
        if (node->angle != value) {
            return 0;
        }
    }
    return 1;
}
