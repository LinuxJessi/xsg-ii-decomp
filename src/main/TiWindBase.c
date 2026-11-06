#include "common.h"
#include "main/TiWindBase.h"
#include "main/tu026.h"

INCLUDE_ASM("asm/main/nonmatchings/TiWindBase", tiWindSolveLink);

INCLUDE_ASM("asm/main/nonmatchings/TiWindBase", func_00297F48);

INCLUDE_ASM("asm/main/nonmatchings/TiWindBase", tiWindClearTables);

INCLUDE_ASM("asm/main/nonmatchings/TiWindBase", tiWindDirSetEnabled);

INCLUDE_ASM("asm/main/nonmatchings/TiWindBase", tiWindDirSetDirection);

INCLUDE_ASM("asm/main/nonmatchings/TiWindBase", tiWindDirSetPosition);

#include "shared.h"
#define DIR_WIND_MAX 4U
#define POINT_WIND_MAX 4U
#define WIND_FLAG_ENABLED 0x1

/*
 * Directional wind table entry, 0x60 bytes.
 * flags: 0x1 enabled, 0x2 global (no falloff with distance).
 * 0x10: unit direction (w = 1 at 0x1C). 0x20: velocity, direction * phase * 0.1.
 * 0x30: position (w = 1 at 0x3C). 0x40: phase, oscillates between upper and lower.
 * phaseStep: +1 or -1, the direction of the phase; speed scales each step.
 * 0x4C: upper bound, 0x50: lower bound, 0x54: radius of the falloff.
 */
typedef struct DirWind {
    int flags;
    unsigned char unmodeled_04[0x40];
    float phaseStep;
    float speed;
    unsigned char unmodeled_4C[0x14];
} DirWind;

/* D_00989D90 is the DirWind table 0x40 bytes in; the speed setter reaches its fields through it. */
typedef struct DirWindValue {
    unsigned char unmodeled_00[4];
    float phaseStep;
    float speed;
    unsigned char unmodeled_0C[0x54];
} DirWindValue;

/*
 * Point wind table entry, 0x40 bytes.
 * flags as DirWind. 0x10: position (w = 1 at 0x1C). 0x24: scalar wave, phase * 0.1.
 * 0x28: phase. phaseStep: +1 or -1. speedRange[0]: speed, [1]: upper bound, [2]: lower bound.
 * 0x3C: radius of the falloff.
 */
typedef struct PointWind {
    int flags;
    unsigned char unmodeled_04[0x28];
    float phaseStep;
    float speedRange[3];
    unsigned char unmodeled_3C[4];
} PointWind;

/* D_00989F00 is the PointWind table 0x30 bytes in; the range setter reaches speedRange[1] and [2] through it. */
typedef struct PointWindValues {
    float speedRange[3];
    unsigned char unmodeled_0C[0x34];
} PointWindValues;

extern DirWind tiWindDirTable[DIR_WIND_MAX];
extern DirWindValue D_00989D90[DIR_WIND_MAX];
extern PointWind tiWindPointTable[POINT_WIND_MAX];
extern PointWindValues D_00989F00[POINT_WIND_MAX];
extern const char D_0068F7B0[];
extern const char D_0068F7C8[];
extern const char D_0068F7E0[];

/* Sets the speed of directional wind `index`. A new speed resets the phase step to +1. */
void tiWindDirSetSpeed(u32 index, float speed)
{
    DirWindValue *entry;

    if (index >= DIR_WIND_MAX) {
        assertFailed(D_0068F7B0, D_0068F7C8, 73);
    }
    entry = &D_00989D90[index];
    if (entry->speed != speed) {
        entry->speed = speed;
        entry->phaseStep = 1.0f;
    }
}

INCLUDE_ASM("asm/main/nonmatchings/TiWindBase", tiWindDirSetRange);

INCLUDE_ASM("asm/main/nonmatchings/TiWindBase", tiWindDirSetRadius);

/* Enables or disables point wind `index`. */
void tiWindPointSetEnabled(u32 index, int enabled)
{
    if (index >= POINT_WIND_MAX) {
        assertFailed(D_0068F7E0, D_0068F7C8, 112);
    }
    if (enabled != 0) {
        tiWindPointTable[index].flags |= WIND_FLAG_ENABLED;
        return;
    }
    tiWindPointTable[index].flags &= ~WIND_FLAG_ENABLED;
}

INCLUDE_ASM("asm/main/nonmatchings/TiWindBase", tiWindPointSetPosition);

/* Sets the speed of point wind `index`, value[0] of its speed and range triple. */
void tiWindPointSetSpeed(u32 index, float speed)
{
    if (index >= POINT_WIND_MAX) {
        assertFailed(D_0068F7E0, D_0068F7C8, 138);
    }
    if (tiWindPointTable[index].speedRange[0] != speed) {
        tiWindPointTable[index].speedRange[0] = speed;
        tiWindPointTable[index].phaseStep = 1.0f;
    }
}

/* Sets the upper and lower phase bounds of point wind `index`. */
void tiWindPointSetRange(u32 index, float upper, float lower)
{
    PointWindValues *entry;

    if (index >= POINT_WIND_MAX) {
        assertFailed(D_0068F7E0, D_0068F7C8, 152);
    }
    entry = &D_00989F00[index];
    entry->speedRange[1] = upper;
    entry->speedRange[2] = lower;
}

INCLUDE_ASM("asm/main/nonmatchings/TiWindBase", tiWindPointSetRadius);

INCLUDE_ASM("asm/main/nonmatchings/TiWindBase", tiWindDirApply);

INCLUDE_ASM("asm/main/nonmatchings/TiWindBase", tiWindPointApply);

/* Pushes the nodes with a directional wind; the two pointer parameters are the step record and the node (main/tu263.c passes its struct pointers through tiWindApplyAll). */
void tiWindDirApply(DirWind *entry, void *first_input, void *second_input);
void tiWindPointApply(PointWind *entry, void *first_input, void *second_input);

/* Applies every enabled wind to the node array; the two pointer parameters are the step record and the node. */
void tiWindApplyAll(void *first_input, void *second_input)
{
    u32 index;

    for (index = 0; index < DIR_WIND_MAX; index++) {
        if (tiWindDirTable[index].flags & WIND_FLAG_ENABLED) {
            tiWindDirApply(&tiWindDirTable[index], first_input, second_input);
        }
    }
    for (index = 0; index < POINT_WIND_MAX; index++) {
        if (tiWindPointTable[index].flags & WIND_FLAG_ENABLED) {
            tiWindPointApply(&tiWindPointTable[index], first_input, second_input);
        }
    }
}

INCLUDE_ASM("asm/main/nonmatchings/TiWindBase", tiWindDirAdvance);

INCLUDE_ASM("asm/main/nonmatchings/TiWindBase", tiWindPointAdvance);

void tiWindDirAdvance(DirWind *entry);
void tiWindPointAdvance(PointWind *entry);

/* Advances the phase of every enabled wind. */
void tiWindAdvanceAll(void)
{
    u32 index;

    for (index = 0; index < DIR_WIND_MAX; index++) {
        if (tiWindDirTable[index].flags & WIND_FLAG_ENABLED) {
            tiWindDirAdvance(&tiWindDirTable[index]);
        }
    }
    for (index = 0; index < POINT_WIND_MAX; index++) {
        if (tiWindPointTable[index].flags & WIND_FLAG_ENABLED) {
            tiWindPointAdvance(&tiWindPointTable[index]);
        }
    }
}

/* Disables every wind; the entries keep their data. */
void tiWindDisableAll(void)
{
    u32 index;

    for (index = 0; index < DIR_WIND_MAX; index++) {
        tiWindDirTable[index].flags &= ~WIND_FLAG_ENABLED;
    }
    for (index = 0; index < POINT_WIND_MAX; index++) {
        tiWindPointTable[index].flags &= ~WIND_FLAG_ENABLED;
    }
}

/* Returns directional wind `index`. */
DirWind *tiWindDirGet(u32 index)
{
    if (index >= DIR_WIND_MAX) {
        assertFailed(D_0068F7B0, D_0068F7C8, 375);
    }
    return &tiWindDirTable[index];
}

/* Returns point wind `index`. */
PointWind *tiWindPointGet(u32 index)
{
    if (index >= POINT_WIND_MAX) {
        assertFailed(D_0068F7E0, D_0068F7C8, 385);
    }
    return &tiWindPointTable[index];
}
