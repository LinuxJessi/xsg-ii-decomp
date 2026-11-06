#include "common.h"

/*
 * The timecode record: the same object as TimecodeObject in main/tu240.c. Its frame
 * count is the first word of that object's stepper (+0x28). Partial record: only the
 * members these accessors touch are named.
 */
typedef struct TimecodeRecord {
    int flags;                    /* 0x00: bit 0x40 is cleared by timecodeClearFlag40 */
    unsigned char unmodeled_04[0x1C - 0x04];
    int state;                    /* 0x1C: the accessors act only while this is 3 */
    unsigned char unmodeled_20[0x28 - 0x20];
    long long elapsedFrames;      /* 0x28: frames at 30 per second */
    unsigned char unmodeled_30[0x38 - 0x30];
    int primary;                  /* 0x38: read back by func_00285AF8 */
    unsigned char unmodeled_3C[0x74 - 0x3C];
    int mirror;                   /* 0x74: written together with primary */
} TimecodeRecord;

/* While active: flags &= ~0x40. */
void timecodeClearFlag40(TimecodeRecord *timecode) {
    if (timecode->state == 3) {
        timecode->flags &= ~0x40;
    }
}

/* While active: elapsedFrames = frames. */
void timecodeSetFrames(TimecodeRecord *timecode, long long frames) {
    if (timecode->state == 3) {
        timecode->elapsedFrames = frames;
    }
}

/* Returns elapsedFrames while active, else 0. */
long long timecodeGetFrames(TimecodeRecord *timecode) {
    if (timecode->state != 3) {
        return 0;
    }
    return timecode->elapsedFrames;
}

/* Splits elapsedFrames into hours, minutes, seconds and frames (30 per second); main 0x00285970. */
INCLUDE_ASM("asm/main/nonmatchings/tu241", timecodeSplit);

/* While active: stores value in primary (+0x38) and mirror (+0x74). */
void timecodeSetPhase(TimecodeRecord *timecode, int value) {
    if (timecode->state == 3) {
        timecode->primary = value;
        timecode->mirror = value;
    }
}

/* Returns primary (+0x38) while active, else 0. */
int func_00285AF8(TimecodeRecord *timecode) {
    if (timecode->state != 3) {
        return 0;
    }
    return timecode->primary;
}

/* Adds the argument to primary and sets flag 0x100; main 0x00285B18. Role of the two callees unread. */
INCLUDE_ASM("asm/main/nonmatchings/tu241", func_00285B18);

/* Starts a timeline for the mode (+0x30): flag 0x200, then func_00285CD0 (mode 0) or func_00285E38 (mode 1 or 2); main 0x00285BF8. */
INCLUDE_ASM("asm/main/nonmatchings/tu241", func_00285BF8);

/* Digit formatter: stores ASCII '0' and '9' into a buffer from a 64-bit value; main 0x00285CD0. Not yet read in full. */
INCLUDE_ASM("asm/main/nonmatchings/tu241", func_00285CD0);

/* Timecode conversion with 60 and 30 constants; main 0x00285E38. Not yet read in full. */
INCLUDE_ASM("asm/main/nonmatchings/tu241", func_00285E38);

/* While active: elapsedFrames = hours * 108000 + minutes * 1800 + seconds * 30, then timecodeSetFrames. */
INCLUDE_ASM("asm/main/nonmatchings/tu241", timecodeSetHms);
