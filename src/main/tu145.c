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
    unsigned char unmodeled_0008[4];
    int firstDirection[2];
    int secondDirection[2];
    unsigned char unmodeled_001C[4];
    struct CfeTrackNode *next;
} CfeTrackNode;

void cfeNormalizeFixed2(int *x, int *y);

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

INCLUDE_ASM("asm/main/nonmatchings/tu145", cfeTrackHasTime);

INCLUDE_ASM("asm/main/nonmatchings/tu145", cfeTrackAnyHasTime);

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

INCLUDE_ASM("asm/main/nonmatchings/tu145", func_00250D88);

INCLUDE_ASM("asm/main/nonmatchings/tu145", cfeTrackCountValueRuns);

INCLUDE_ASM("asm/main/nonmatchings/tu145", func_00250E10);

INCLUDE_ASM("asm/main/nonmatchings/tu145", cfeTrackShiftTimes);

INCLUDE_ASM("asm/main/nonmatchings/tu145", func_00250EF0);

INCLUDE_ASM("asm/main/nonmatchings/tu145", cfeTrackAllValuesEqual);
