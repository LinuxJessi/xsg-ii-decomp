#include "common.h"

INCLUDE_ASM("asm/main/nonmatchings/tu145", cfeNormalizeFixed2);

#include "shared.h"

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

void cfeNormalizeFixed2(int *x, int *y);
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

INCLUDE_ASM("asm/main/nonmatchings/tu145", cfeChainUnwrapAngles);

INCLUDE_ASM("asm/main/nonmatchings/tu145", cfeTrackLerp);

extern f32 cfeTimeScale;

/* Cubic in the time elapsed since the key (scaled by cfeTimeScale): coefficients[0..3], Horner form. */
f32 cfeEntryEvalCubic(CfeTrackNode *entry, f32 *coefficients, f32 time)
{
    f32 elapsed = (time - (f32) entry->startTime) * cfeTimeScale;

    return (elapsed * ((elapsed * ((elapsed * coefficients[0]) + coefficients[1])) + coefficients[2])) + coefficients[3];
}

INCLUDE_ASM("asm/main/nonmatchings/tu145", func_00250478);

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

INCLUDE_ASM("asm/main/nonmatchings/tu145", func_00250C68);

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

INCLUDE_ASM("asm/main/nonmatchings/tu145", func_00250E10);

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

INCLUDE_ASM("asm/main/nonmatchings/tu145", func_00250EF0);

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
